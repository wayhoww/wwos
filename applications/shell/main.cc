#include "wwos/format.h"
#include "wwos/pair.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string.h"
#include "wwos/syscall.h"
#include "wwos/vector.h"


// built-in command: 
// ls
// pwd
// info
// cat
// help


wwos::string getline() {
    wwos::string out;
    while(true) {
        wwos::int32_t c = wwos::getchar();
        if(c == -1) {
            continue;
        }
        wwos::putchar(c);

        if(c == '\r') {
            wwos::putchar('\n');
            break;
        }

        out.push_back(c);
    }

    return out;
}

wwos::vector<wwos::string> tokenize(wwos::string str) {
    wwos::vector<wwos::string> out;
    wwos::string current;
    for(auto c: str) {
        if(c == ' ') {
            if(current.size() > 0) {
                out.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if(current.size() > 0) {
        out.push_back(current);
    }
    return out;
}

struct command_invocation {
    wwos::string command;
    wwos::vector<wwos::string> args;
    wwos::string input;
    wwos::string output;
};

// syntax:
// statement: command args io
// command: string
// args: string*
// io: input | output | input output | output input
// input: '<' string
// output: '>' string

// ll parser

class statement_parser {
public:
    statement_parser(const wwos::vector<wwos::string>& tokens): tokens(tokens) {}

    bool success() {
        return succ;
    }

    wwos::string error() {
        return error_message;
    }

    command_invocation parse() {
        command_invocation out;

        out.command = expect_command();
        if(!success()) return {};

        while(true) {
            if(m_pos >= tokens.size()) {
                break;
            }

            if(tokens[m_pos] == "<" || tokens[m_pos] == ">") {
                break;
            }

            auto arg = expect_arg();
            if(!success()) return {};
            out.args.push_back(arg);
        }

        if(m_pos >= tokens.size()) {
            return out;
        }

        for(wwos::size_t i = 0; i < 2; i++) {
            if(m_pos >= tokens.size()) {
                break;
            }
            auto redirect = expect_redirect();
            if(!success()) return {};
            if(redirect == "<") {
                if(out.input.size() > 0) {
                    error_message = "Duplicate input redirect";
                    succ = false;
                    return {};
                }
                out.input = expect_file();
                if(!success()) return {};
            } else {
                if(out.output.size() > 0) {
                    error_message = "Duplicate output redirect";
                    succ = false;
                    return {};
                }
                out.output = expect_file();
                if(!success()) return {};
            }
        }

        return out;
    }

    wwos::string expect_redirect() {
        if(m_pos >= tokens.size()) {
            error_message = "Expected redirect";
            succ = false;
            return "";
        }

        if(tokens[m_pos] != "<" && tokens[m_pos] != ">") {
            error_message = "Expected redirect";
            succ = false;
            return "";
        }

        return tokens[m_pos++];
    }

    wwos::string expect_command() {
        return expect_string();
    }

    wwos::string expect_file() {
        return expect_string();
    }

    wwos::string expect_arg() {
        return expect_string();
    }

    wwos::string expect_string() {
        if(m_pos >= tokens.size()) {
            error_message = "Expected name";
            succ = false;
            return "";
        }

        if(tokens[m_pos] == "<" || tokens[m_pos] == ">") {
            error_message = "Expected name";
            succ = false;
            return "";
        }

        return tokens[m_pos++];
    }

private:
    wwos::string error_message;
    wwos::vector<wwos::string>  tokens;
    bool succ = true;
    wwos::size_t m_pos = 0;
};

void dump_command(command_invocation cmd) {
    wwos::printf("Command: {}\n", cmd.command);
    wwos::println("Args:");
    for(auto& arg: cmd.args) {
        wwos::printf("  {}\n", arg);
    }
    wwos::printf("Input: {}\n", cmd.input);
    wwos::printf("Output: {}\n", cmd.output);
}

void command_help(const wwos::vector<wwos::string>& args) {
    wwos::println("Syntax: command args ... [> output] [< input]");
    wwos::println("Commands:");
    wwos::println("  ls");
    wwos::println("  cat");
    wwos::println("  mkdir");
    wwos::println("  clear");
    wwos::println("  help");
}

void command_clear(const wwos::vector<wwos::string>& args) {
    wwos::putchar(0x1b);
    wwos::putchar(0x5b);
    wwos::putchar(0x33);
    wwos::putchar(0x4a);
    wwos::putchar(0x1b);
    wwos::putchar(0x5b);
    wwos::putchar(0x48);
    wwos::putchar(0x1b);
    wwos::putchar(0x5b);
    wwos::putchar(0x32);
    wwos::putchar(0x4a);
}

void command_ls(const wwos::vector<wwos::string>& args) {
    if(args.size() != 1) {
        wwos::println("Usage: ls <path>");
        return;
    }

    wwos::int64_t fid = wwos::open(args[0]);
    if(fid < 0) {
        wwos::printf("Failed to open {}\n", args[0]);
        return;
    }
    auto children = wwos::get_directory_children(fid);
    if(children.size() == 0) {
        wwos::printf("No file or directories in {}\n", args[0]);
        return;
    } else {
        for(auto& child: children) {
            wwos::printf("{}\t", child.first);
        }
        wwos::println("");
    }
}

void command_mkdir(const wwos::vector<wwos::string>& args) {
    if(args.size() != 1) {
        wwos::println("Usage: mkdir <path>");
        return;
    }

    // find the last '/', split
    wwos::string path = args[0];
    wwos::size_t last_slash = path.find_last_of('/');
    if(last_slash == wwos::string::npos) {
        wwos::println("Invalid path");
        return;
    }

    wwos::string parent = path.substr(0, last_slash + 1);
    wwos::string name = path.substr(last_slash + 1, path.size() - last_slash - 1);

    wwos::int64_t parent_fid = wwos::open(parent);
    if(parent_fid < 0) {
        wwos::printf("Failed to open {}\n", parent);
        return;
    }

    wwos::int64_t new_fid = wwos::make_directory(parent_fid, name);
    if(new_fid < 0) {
        wwos::printf("Failed to create {}\n", args[0]);
        return;
    }
}

int main() {
    wwos::println("WWOS Shell!");

    // wwos::int64_t root = wwos::open("/");
    // wwos::printf("Root: {}\n", root);

    // auto children = wwos::get_directory_children(root);
    
    // for(auto& child: children) {
    //     wwos::printf("Child: {} {}\n", child.first, child.second);
    // }

    while(true) {
        wwos::print("> ");
        auto line = getline();
        auto tokens = tokenize(line);
        auto parser = statement_parser(tokens);
        auto cmd = parser.parse();

        if(!parser.success()) {
            wwos::println(parser.error());
            continue;
        } 

        if(cmd.command == "ls") {
            command_ls(cmd.args);
        } else if(cmd.command == "mkdir") {
            command_mkdir(cmd.args);
        } else if(cmd.command == "help") {
            command_help(cmd.args);
        } else if(cmd.command == "clear") {
            command_clear(cmd.args);
        } else {
            for(auto c: line) {
                wwos::printf("{} ", int(c));
            }
            wwos::println("");
            for(auto c: cmd.command) {
                wwos::printf("{} ", int(c));
            }
            wwos::println("");
            wwos::printf("Unknown command: #{}#\n", cmd.command);
        }
    }


    return 0;
}