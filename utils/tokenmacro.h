/* This is derived from LuaJit implementation                              */
#define TKDEF(_, __, ___)                                                                                              \
    _(and) _(break) _(do) _(else) _(elseif) _(end) \
    _(false) _(for) _(function) _(goto) _(if) _(in) _(local) \
    _(defer) _(C) /* Ravi extensions */ \
    _(nil) _(not) _(or) \
    _(repeat) _(return) _(then) _(true) _(until) _(while) \
    /* other terminal symbols */ \
    ___(IDIV, /) __(CONCAT, ..) __(DOTS, ...) __(EQ, ==) \
    __(GE, >=) __(LE, <=) __(NE, ~=) __(SHL, <<) \
    __(SHR, >>) __(DBCOLON, ::) \
    /** RAVI extensions */ \
    __(TO_INTEGER, @integer) __(TO_NUMBER, @number) \
    __(TO_INTARRAY, @integer[]) __(TO_NUMARRAY, @number[]) \
    __(TO_TABLE, @table) __(TO_STRING, @string) __(TO_CLOSURE, @closure) __(EOS, <eof>) \
    /* Tokens below this populate the seminfo */ \
    __(FLT, <number>) __(INT, <integer>) __(NAME, <name>) __(STRING, <string>)