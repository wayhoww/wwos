#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glib.h"
#include "qemu-plugin.h"


QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;


struct qemu_plugin_register* handle_reg_pc = NULL;
struct qemu_plugin_register* handle_reg_esr_el1 = NULL;

unsigned long read_register(struct qemu_plugin_register* reg) {
    GByteArray* buf = g_byte_array_new();
    qemu_plugin_read_register(reg, buf);
    if(buf->len != 8) {
        g_byte_array_unref(buf);
        return 0xdeadbeefdeadbeef;
    }
    unsigned long val = (*(uint64_t*)buf->data);
    g_byte_array_unref(buf);
    return val;
}

void instruction_executed_callback(unsigned int vcpu_index, void *userdata) {
    if(handle_reg_pc == NULL) {
        return;
    }
    
    unsigned long pc = read_register(handle_reg_pc);
    
    char buffer[64];
    snprintf(buffer, 32, "PC: %lx\n", pc);
    qemu_plugin_outs(buffer);
    

}

void translation_block_executed_callback(qemu_plugin_id_t id, struct qemu_plugin_tb *tb) {
    int n_insts = qemu_plugin_tb_n_insns(tb);
    for(int i = 0; i < n_insts; i++) {
        struct qemu_plugin_insn* insn = qemu_plugin_tb_get_insn(tb, i);
        qemu_plugin_register_vcpu_insn_exec_cb(insn, instruction_executed_callback, QEMU_PLUGIN_CB_R_REGS, (void*)(long)i );
    }
    qemu_plugin_outs("Translation block executed\n");
}

void virtual_cpu_init_callback(qemu_plugin_id_t id, unsigned int vcpu_index) {
    GArray* registers = qemu_plugin_get_registers();
    
    qemu_plugin_reg_descriptor* data = (qemu_plugin_reg_descriptor*) registers->data;
    for(int i = 0; i < registers->len; i++) {
        if(strcmp(data[i].name, "pc") == 0) {
            handle_reg_pc = data[i].handle;
        }
        if(strcmp(data[i].name, "ESR_EL1") == 0) {
            handle_reg_esr_el1 = data[i].handle;
        }
    }


    free(registers);
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc, char **argv) {
    qemu_plugin_register_vcpu_tb_trans_cb(id, translation_block_executed_callback);
    qemu_plugin_register_vcpu_init_cb(id, virtual_cpu_init_callback);
    return 0;
}