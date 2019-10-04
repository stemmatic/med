
#include <stdio.h>
#include "med.h"

static struct Ctx *
	ctxNew()
{
	static struct Ctx ctx[1];
	ctx->active = 0;
	return ctx;
}

void
	copyUntil(int delim)
{
	int c;
	while ((c = getchar()) != EOF && c != delim)
		putchar(c);
	if (c != EOF)
		putchar(c);
}

static void
	filter(struct Ctx *ctx)
{
	int c;

	while ((c = getchar()) != EOF) {
		if (ctx->active) {
			special(ctx, c);
			continue;
		}

		switch (c) {
		default:
			putchar(c);
			break;
		case '{':
			ctx->active = activate(ctx);
			break;
		case '"':
			putchar(c);
			copyUntil(c);
			break;
		}
	}
}

int
	main(int argc, char *argv[])
{
	struct Ctx *ctx;

	if (argc != 1) {
		fprintf(stderr, "Usage: %s < in-file > out->file\n", argv[0]);
		return -1;
	}

	ctx = ctxNew();
	filter(ctx);

	return 0;
}
