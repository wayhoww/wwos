#include "wwos/syscall.h"
#include "wwos/assert.h"
#include "wwos/stdint.h"

namespace wwos {

    void parse_null_terminated_strings(vector<string>& out, const char* buffer, size_t size) {
        size_t i = 0;
        while(i < size) {
            string name(buffer + i);
            if(name.size() == 0) {
                break;
            }
            i += name.size() + 1;
            out.push_back(name);
        }
    }

    int64_t get_children(int64_t fd, vector<string>& out) {
        vector<uint8_t> buffer(4096);
        uint64_t params[] = {static_cast<uint64_t>(fd), reinterpret_cast<uint64_t>(buffer.data()), buffer.size()};
        int64_t ret = syscall(syscall_id::FD_CHILDREN, reinterpret_cast<uint64_t>(params));

        if(ret < 0) {
            return ret;
        }

        int counter = 0;

        while (ret > 0) {
            counter += 1;
            wwassert(counter < 1000, "Too many retries");

            buffer = vector<uint8_t>(ret);
            params[1] = reinterpret_cast<uint64_t>(buffer.data());
            params[2] = ret;
            ret = syscall(syscall_id::FD_CHILDREN, reinterpret_cast<uint64_t>(params));
            if(ret < 0) {
                return ret;
            }
        }

        parse_null_terminated_strings(out, reinterpret_cast<const char*>(buffer.data()), buffer.size());
        return 0;
    }
}