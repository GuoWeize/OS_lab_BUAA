#include "regdef.h"
#include "cp0regdef.h"

/* 声明 leaf 例程 */
#define LEAF(symbol)								\
			.globl  symbol;                         \
			.align  2;                              \
			.type   symbol,@function;               \
			.ent    symbol,0;                       \
symbol:		.frame  sp,0,ra

/* 声明 nested 例程入口 */
#define NESTED(symbol, framesize, rpc)				\
			.globl  symbol;                         \
			.align  2;                              \
			.type   symbol,@function;               \
			.ent    symbol,0;                       \
symbol:		.frame  sp, framesize, rpc


/* 标记 function 结束 */
#define END(function)						\
		.end    function;					\
		.size   function,.-function


#define EXPORT(symbol)					\
		.globl  symbol;					\
symbol:


#define FEXPORT(symbol)						\
		.globl  symbol;						\
		.type   symbol,@function;			\
symbol:
