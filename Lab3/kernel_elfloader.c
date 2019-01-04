#include "kerelf.h"
#include "types.h"
#include "pmap.h"

/*  判断是不是一个 ELF 文件
先前条件: binary 必须长于 4 字节
结束状态: 是返回 1, 不是返回 0 */
int is_elf_format(u_char *binary) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;

	if (ehdr->e_ident[0] == ELFMAG0 &&
		ehdr->e_ident[1] == ELFMAG1 &&
		ehdr->e_ident[2] == ELFMAG2 &&
		ehdr->e_ident[3] == ELFMAG3) {
		return 0;
	}

	return 1;
}


/*  加载一个 ELF 文件, 将所有的 section 映射到正确的虚地址, 传入 load_icode_mapper 和参数 user_data 以完成
先前条件:
	binary 不能为空, size 是 binary 的大小
结束状态:
	如果成功返回 0, 否则返回负值
	如果成功, 解析出的入口地址会被存入到 entry_point, 其为一个 u_long 变量的地址(相当于引用) */
int load_elf(u_char *binary, int size, u_long *entry_point, void *user_data,
			 int (*map)(u_long va, u_int32_t sgsize,
						u_char *bin, u_int32_t bin_size, void *user_data)) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	Elf32_Phdr *phdr = (Elf32_Phdr *)NULL;
	// 作为加载器, 只考虑 segment, 所以只解析程序头部
	u_char *ptr_ph_table = (u_char *)NULL;
	Elf32_Half ph_entry_count;
	Elf32_Half ph_entry_size;
	int r;

	// 检查 binary 是不是 ELF 文件
	if (size < 4 || !is_elf_format(binary)) {
		return -1;
	}

	ptr_ph_table = binary + ehdr->e_phoff;
	ph_entry_count = ehdr->e_phnum;
	ph_entry_size = ehdr->e_phentsize;

	while (ph_entry_count--) {
		phdr = (Elf32_Phdr *)ptr_ph_table;

		if (phdr->p_type == PT_LOAD) {
			/* 将所有 section 真正映射到正确的虚地址, 如果出错返回负值 */
			r = map(phdr->p_vaddr, phdr->p_memsz,
					phdr->p_offset + binary, phdr->p_filesz, user_data);
			if (r != 0)
				return r;
		}

		ptr_ph_table += ph_entry_size;
	}

	*entry_point = ehdr->e_entry;
	return 0;
}
