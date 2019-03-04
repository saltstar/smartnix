
#pragma once

#include "abigen_generator.h"
#include "parser/parser.h"

bool process_comment(AbigenGenerator* parser, TokenStream& ts);
bool process_syscall(AbigenGenerator* parser, TokenStream& ts);
