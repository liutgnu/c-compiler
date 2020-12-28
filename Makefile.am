noinst_PROGRAMS = out/lex.out
noinst_PROGRAMS += out/parser.out

noinst_LIBRARIES = out/liblex.a
noinst_LIBRARIES += out/libparser.a

out_liblex_a_SOURCES = src/lexer/lex.c
out_liblex_a_SOURCES += src/lexer/lex.h
out_liblex_a_SOURCES += src/lexer/token.h

out_libparser_a_SOURCES = src/parser/ast_token.c
out_libparser_a_SOURCES += src/parser/ast_token.h
out_libparser_a_SOURCES += src/parser/ast_tree_view.c
out_libparser_a_SOURCES += src/parser/ast_tree_view.h
out_libparser_a_SOURCES += src/parser/linked_list_buffer.c
out_libparser_a_SOURCES += src/parser/linked_list_buffer.h
out_libparser_a_SOURCES += src/parser/parse.c
out_libparser_a_SOURCES += src/parser/parse.h

out_lex_out_SOURCES = src/lexer/main.c
out_parser_out_SOURCES = src/parser/main.c

out_lex_out_LDADD = out/liblex.a
out_parser_out_LDADD = out/liblex.a
out_parser_out_LDADD += out/libparser.a
out_parser_out_LDADD += -lpthread
