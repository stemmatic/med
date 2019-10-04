
typedef int Flag;
#define NO  0
#define YES 1

#define EOS '\0'

enum CState { csLemma, csVariants, csOther, };

struct Ctx {
	struct Special *active;
};

extern void copyUntil(int delim);
extern void special(struct Ctx *ctx, int c);
extern struct Special *activate(struct Ctx *ctx);
