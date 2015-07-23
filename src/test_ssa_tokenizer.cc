// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa_tokenizer.h"
#include <cassert>
#include <sstream>
#include <vector>

using hcc::ssa::token;
using hcc::ssa::token_type;
using hcc::ssa::tokenizer;

auto example = R"(
define @Sys.init
block $1
	call %0_0 @Sys.identity 42 ;
	store @Sys.a %0_0 ;
	return 0 ;
define @Sys.identity
	block $1
	argument %x_0 0 ;
	return %x_0 ;
)";
void test_example()
{
    std::istringstream iss {example};
    hcc::ssa::tokenizer t {iss};
    std::vector<hcc::ssa::token> tokens;
    for (int i = 0; i < 29; ++i) {
        tokens.push_back(t.next());
    }
    assert (tokens[ 0].type == token_type::DEFINE);
    assert (tokens[ 1].type == token_type::GLOBAL);
    assert (tokens[ 1].token == "Sys.init");
    assert (tokens[ 2].type == token_type::BLOCK);
    assert (tokens[ 3].type == token_type::LABEL);
    assert (tokens[ 3].token == "1");
    assert (tokens[ 4].type == token_type::CALL);
    assert (tokens[ 5].type == token_type::REG);
    assert (tokens[ 5].token == "0_0");
    assert (tokens[ 6].type == token_type::GLOBAL);
    assert (tokens[ 6].token == "Sys.identity");
    assert (tokens[ 7].type == token_type::CONSTANT);
    assert (tokens[ 7].token == "42");
    assert (tokens[ 8].type == token_type::SEMICOLON);
    assert (tokens[ 9].type == token_type::STORE);
    assert (tokens[10].type == token_type::GLOBAL);
    assert (tokens[10].token == "Sys.a");
    assert (tokens[11].type == token_type::REG);
    assert (tokens[11].token == "0_0");
    assert (tokens[12].type == token_type::SEMICOLON);
    assert (tokens[13].type == token_type::RETURN);
    assert (tokens[14].type == token_type::CONSTANT);
    assert (tokens[14].token == "0");
    assert (tokens[15].type == token_type::SEMICOLON);
    assert (tokens[16].type == token_type::DEFINE);
    assert (tokens[17].type == token_type::GLOBAL);
    assert (tokens[17].token == "Sys.identity");
    assert (tokens[18].type == token_type::BLOCK);
    assert (tokens[19].type == token_type::LABEL);
    assert (tokens[19].token == "1");
    assert (tokens[20].type == token_type::ARGUMENT);
    assert (tokens[21].type == token_type::REG);
    assert (tokens[21].token == "x_0");
    assert (tokens[22].type == token_type::CONSTANT);
    assert (tokens[22].token == "0");
    assert (tokens[23].type == token_type::SEMICOLON);
    assert (tokens[24].type == token_type::RETURN);
    assert (tokens[25].type == token_type::REG);
    assert (tokens[25].token == "x_0");
    assert (tokens[26].type == token_type::SEMICOLON);
    assert (tokens[27].type == token_type::END);
}

int main()
{
    test_example();
    return 0;
}
