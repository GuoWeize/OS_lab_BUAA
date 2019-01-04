#include "kerelf.h"
#include "types.h"
#include "pmap.h"

/*  �ж��ǲ���һ�� ELF �ļ�
��ǰ����: binary ���볤�� 4 �ֽ�
����״̬: �Ƿ��� 1, ���Ƿ��� 0 */
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


/*  ����һ�� ELF �ļ�, �����е� section ӳ�䵽��ȷ�����ַ, ���� load_icode_mapper �Ͳ��� user_data �����
��ǰ����:
	binary ����Ϊ��, size �� binary �Ĵ�С
����״̬:
	����ɹ����� 0, ���򷵻ظ�ֵ
	����ɹ�, ����������ڵ�ַ�ᱻ���뵽 entry_point, ��Ϊһ�� u_long �����ĵ�ַ(�൱������) */
int load_elf(u_char *binary, int size, u_long *entry_point, void *user_data,
			 int (*map)(u_long va, u_int32_t sgsize,
						u_char *bin, u_int32_t bin_size, void *user_data)) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	Elf32_Phdr *phdr = (Elf32_Phdr *)NULL;
	// ��Ϊ������, ֻ���� segment, ����ֻ��������ͷ��
	u_char *ptr_ph_table = (u_char *)NULL;
	Elf32_Half ph_entry_count;
	Elf32_Half ph_entry_size;
	int r;

	// ��� binary �ǲ��� ELF �ļ�
	if (size < 4 || !is_elf_format(binary)) {
		return -1;
	}

	ptr_ph_table = binary + ehdr->e_phoff;
	ph_entry_count = ehdr->e_phnum;
	ph_entry_size = ehdr->e_phentsize;

	while (ph_entry_count--) {
		phdr = (Elf32_Phdr *)ptr_ph_table;

		if (phdr->p_type == PT_LOAD) {
			/* ������ section ����ӳ�䵽��ȷ�����ַ, ��������ظ�ֵ */
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
