#include "wwos/assert.h"
#include "wwos/format.h"
#include "wwos/list.h"
#include "wwos/queue.h"
#include "wwos/stdint.h"
#include "wwos/stdio.h"
#include "wwos/string_view.h"
#include "wwos/syscall.h"
#include "wwos/vector.h"


wwos::int64_t current_tty = -1;

class output_proxy {
public:
    output_proxy(): m_buffer(1024) {}

    void write(wwos::uint8_t c) {
        wwos::kputchar(c);
        wwos::uint8_t buf;
        if(m_buffer.full()) m_buffer.pop(buf);
        m_buffer.push(c);
    }

    void rewrite() {
        for(auto c: m_buffer.items()) {
            wwos::kputchar(c);
        }
    }

    void clear() {
        m_buffer.clear();
    }

    void pop(wwos::size_t n) {
        for(wwos::size_t i = 0; i < n; i++) {
            wwos::uint8_t c;
            m_buffer.pop_back(c);
        }
    }

protected:
    wwos::cycle_queue<wwos::uint8_t> m_buffer;
};

struct tty_info {
    wwos::int64_t pid;
    wwos::int64_t fd_stdin;
    wwos::int64_t fd_stdout;
    wwos::cycle_queue<wwos::uint8_t> buffer_stdin  { 1 << 20 };
    wwos::cycle_queue<wwos::uint8_t> buffer_stdout { 1 << 20 };
    wwos::vector<wwos::uint8_t> buffer_line;
    output_proxy proxy;
};

constexpr wwos::size_t N_TTYS = 4;
tty_info* ttys[N_TTYS + 1];
wwos::size_t BUFFER_SIZE = 1 << 20;

wwos::string tty_getline(output_proxy& proxy) {
    wwos::string out;
    while(true) {
        wwos::int32_t c = wwos::kgetchar();
        if(c == -1) {
            continue;
        }
        proxy.write(c);

        if(c == '\r') {
            proxy.write('\n');
            break;
        }

        out.push_back(c);
    }

    return out;
}

void tty_print(output_proxy& proxy, wwos::string_view s) {
    for (wwos::size_t i = 0; i < s.size(); i++) {
        proxy.write(s[i]);
    }
}

void initialize_tty_log() {
    if(ttys[N_TTYS] != nullptr) {
        return;
    }

    ttys[N_TTYS] = new tty_info {
        .pid = 0,
        .fd_stdin = -1,
        .fd_stdout = wwos::open("/kernel/log", wwos::fd_mode::READONLY),
        .buffer_stdin = wwos::cycle_queue<wwos::uint8_t>(BUFFER_SIZE),
        .buffer_stdout = wwos::cycle_queue<wwos::uint8_t>(BUFFER_SIZE)
    };

    wwassert(ttys[N_TTYS]->fd_stdout >= 0, "Failed to open fifo");
}

void initialize_tty(wwos::size_t i) {
    if(i == N_TTYS) {
        initialize_tty_log();
        return;
    }

    if(ttys[i] != nullptr) {
        return;
    }

    auto pid = wwos::fork();
    if(pid < 0) {
        wwassert(false, "Failed to fork");
        return;
    }

    if(pid == 0) {
        wwos::exec("/app/shell");
    }

    ttys[i] = new tty_info {
        .pid = pid,
        .fd_stdin = wwos::open(wwos::format("/proc/{}/fifo/stdin", pid), wwos::fd_mode::WRITEONLY),
        .fd_stdout = wwos::open(wwos::format("/proc/{}/fifo/stdout", pid), wwos::fd_mode::READONLY),
        .buffer_stdin = wwos::cycle_queue<wwos::uint8_t>(BUFFER_SIZE),
        .buffer_stdout = wwos::cycle_queue<wwos::uint8_t>(BUFFER_SIZE)
    };

    wwassert(ttys[i]->fd_stdin >= 0, "Failed to open fifo");
    wwassert(ttys[i]->fd_stdout >= 0, "Failed to open fifo");
}

void switch_to_tty(wwos::int64_t i) {
    if(i == current_tty) {
        return;
    }
    if(ttys[i] == nullptr) {
        initialize_tty(i);
    }
    
    current_tty = i;
}

void clear_screen() {
    wwos::kputchar(0x1b);
    wwos::kputchar(0x5b);
    wwos::kputchar(0x33);
    wwos::kputchar(0x4a);
    wwos::kputchar(0x1b);
    wwos::kputchar(0x5b);
    wwos::kputchar(0x48);
    wwos::kputchar(0x1b);
    wwos::kputchar(0x5b);
    wwos::kputchar(0x32);
    wwos::kputchar(0x4a);
}

void command_mode(output_proxy& proxy) {
    tty_print(proxy, "WWOS Virtual TTY Service\n");
    tty_print(proxy, "-----------------------------------------------\n");
    tty_print(proxy, "goto <id>      Switch to TTY #id. id = 1/2/3/4 \n");
    tty_print(proxy, "log            Show system log                 \n");
    tty_print(proxy, "ret            Return to previous tty          \n");
    tty_print(proxy, "\n");
    tty_print(proxy, "\n");

    
    while(true) {
        tty_print(proxy, "vtty> ");
        auto line = tty_getline(proxy);
        if(line == "log") {
            switch_to_tty(N_TTYS);
            return;
        } else if(line == "ret") {
            return;
        } else if(line.starts_with("goto ")) {
            auto id_string = line.substr(5, line.size() - 5);
            wwos::int32_t id;
            auto succ = wwos::stoi(id_string, id);
            if(!succ) {
                tty_print(proxy, "Failed to parse id\n");
                continue;
            }
            if(id < 1 || id > 4) {
                tty_print(proxy, "Invalid id\n");
                continue;
            }
            switch_to_tty(id - 1);
            return;
        } else {
            tty_print(proxy, "Invalid command\n");
            continue;
        }
    }
}


bool is_normal_characters(wwos::int32_t c) {
    if(c >= 32 && c <= 136) {
        return true;
    }
    return false;
}

int main() {
    for(wwos::size_t i = 0; i < N_TTYS + 1; i++) {
        ttys[i] = nullptr;
    }

    current_tty = -1;
    switch_to_tty(N_TTYS);
    switch_to_tty(0);

    clear_screen();

    output_proxy command_mode_proxy;
    
    while(true) {
        auto c = wwos::kgetchar();
        
        if(c == 27) {
            clear_screen();
            command_mode(command_mode_proxy);

            auto p_tty = ttys[current_tty];
            clear_screen();
            p_tty->proxy.rewrite();
            continue;
        }


        auto p_tty = ttys[current_tty];

        if(current_tty != N_TTYS) {
            if(c == 127) {
                if(p_tty->buffer_line.size() > 0) {
                    p_tty->proxy.pop(1);
                    p_tty->buffer_line.pop_back();
                    clear_screen();
                    p_tty->proxy.rewrite();
                }
            } else if(is_normal_characters(c) || c == 13) {
                if(c == 13) c = 10;

                p_tty->buffer_line.push_back(c);
                p_tty->proxy.write(c);
            

                if(c == 10) {
                    for(auto c: p_tty->buffer_line) {
                        if(p_tty->buffer_stdin.full()) {
                            wwos::uint8_t buf;
                            p_tty->buffer_stdin.pop(buf);
                        }
                        p_tty->buffer_stdin.push(c);
                    }
                    p_tty->buffer_line.clear();
                }
            }
            
            while(true) {
                wwos::uint8_t bufc;
                bool succ = p_tty->buffer_stdin.get_front(bufc);
                if(!succ) {
                    break;
                }
                auto size = wwos::write(p_tty->fd_stdin, &bufc, 1);
                if(size <= 0) {
                    break;
                }
                p_tty->buffer_stdin.pop(bufc);
            }
        }

        for(wwos::size_t i = 0; i < N_TTYS + 1; i++) {
            auto i_tty = ttys[i];
            if(i_tty == nullptr) {
                continue;
            }
        
            char buffer[128];
            while(true) {
                auto size = wwos::read(i_tty->fd_stdout, (wwos::uint8_t*)buffer, sizeof(buffer));
                
                for(wwos::int64_t i = 0; i < size; i++) {
                    if(i_tty->buffer_stdout.full()) {
                        wwos::uint8_t buf;
                        i_tty->buffer_stdout.pop(buf);
                    }
                    i_tty->buffer_stdout.push(buffer[i]);
                }

                if(size < sizeof(buffer)) {
                    break;
                }
            }
        }

        while(true) {
            wwos::uint8_t bufc;
            bool succ = p_tty->buffer_stdout.get_front(bufc);
            if(!succ) {
                break;
            }
            p_tty->proxy.write(bufc);
            p_tty->buffer_stdout.pop(bufc);
        }
    }
    return 0;
}