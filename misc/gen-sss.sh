#!/bin/sh
set -eu; set -- "$0" --ba-- "$0" "$@" --ea--
exec nofake-exec.sh --error -Rprog "$@" -- icon
exit 1

TODO: add option to align

static string set

<<usage example>>=
./misc/gen-sss.sh g_sss_skip __restrict __restrict__ _Noreturn __inline __extension__ __const __wur __asm__
@

<<prog>>=
$line 13
procedure main(args)
    local prgname, idx, str, sym, symslst, symstbl
    prgname := pop(args)
    *args < 2 & stop("usage: ", prgname, " name member1 member2 ... memberN")
    name := pop(args)
    write("static const char ", name, "[] =")
    every write("    \042", !args, "\\000\042")
    write(";")
    idx := 0
    symslst := []
    symstbl := table(0)
    every !args ? {
        str := &subject
        every tab(upto(' ')) do {
            &subject[&pos] := "_"
        }
        sym := &subject
        put(symslst, sym)
        symstbl[sym] := str
        write("const char *g_str_", sym, " = ", name, " + ", idx, ";")
        idx +:= *sym + 1
    }
    write("static const char *", name, "_end = ", name, " + ", idx, ";")
    write("int is_", name, "_member(const char *s)")
    write("{")
    write("    return s >= ", name, " && s <= ", name, "_end;")
    write("}")

    write()
    every sym := !symslst do
        write("spec_str((char*)g_str_", sym, ");")

    write()
    every sym := !symslst do
        write("extern const char *g_str_", sym, ";")
end
