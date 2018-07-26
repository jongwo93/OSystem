import argparse
import struct
import pdb
import re
import os.path

"""
    lc2200hw1-sim.py: A basic simulator for the LC-2200 architecture for Homework 1 of Spring 2017.
    Adapted from Fall 2016 simulator made by Christopher Tam.
"""
__author__ = "Adithya Nott"


# Python 2: remap input to raw_input
try:
   input = raw_input
except NameError:
   pass

PC = 0
MEM = {}
REGS = {0:0, 1:0, 2:0, 3:0, 4:0, 5:0, 6:0, 7:0,
        8:0, 9:0, 10:0, 11:0, 12:0, 13:0, 14:0, 15:0}
#CC = {'N': 0, 'Z': 1, 'P': 0}

OP_NAME = {'000':'ADD', '001':'NAND', '010':'ADDI', '011':'LW',
           '100':'SW', '101':'BEQ', '110':'JALR', '111':'HALT'}
R_TYPES = {'ADD', 'NAND'}
I_TYPES = {'ADDI', 'LW', 'SW', 'BEQ'}
J_TYPES = {'JALR'}
O_TYPES = {'HALT'}

REG_NAME = {'0000':'$zero', '0001':'$at', '0010':'$v0', '0011':'$a0',
           '0100':'$a1', '0101':'$a2', '0110':'$t0', '0111':'$t1',
           '1000':'$t2', '1001':'$s0', '1010':'$s1', '1011':'$s2',
           '1100':'$k0', '1101':'$sp', '1110':'$fp', '1111':'$ra'}
BREAKPOINTS = []
LABELS = {}

HALTED = False
QUIT = False

LAST_CMD = 'help'

def uint(binary):
    return int(binary, 2)

def int2c(binary):
    return force_signed(uint(binary), len(binary))

def truncate_int(num, bits):
    return num % (2 ** bits)

def force_signed(num, bits):
    num = truncate_int(num, bits)
    if (1 << bits - 1) & num:
        return num - (1 << bits)
    else:
        return num

def force_unsigned(num, bits):
    return ((1 << bits) - 1) & num

def load_program(runfile):
    addr = 0
    with open(runfile, 'r') as f:
        for line in f:
            # parse binary string into integer and strip newline
            MEM[addr] = int(line[:-1], 2)
            addr += 1

    symfile = runfile.split('.')
    symfile = symfile[0] + '.sym'
    if os.path.isfile(symfile):
        with open(symfile, 'r') as f:
            for line in f:
                symbol_regex = '(\w+): 0[xX]([0-9a-fA-F]+)'
                symbol_m = re.match(symbol_regex, line)
                label = symbol_m.group(1)
                addr = int(symbol_m.group(2), 16)
                LABELS[addr] = label
    else:
        print("WARNING: No symbol file available at {}, continuing without symbolic debugging".format(symfile))

def access_mem(addr):
    max_addr = 2 ** 24
    if (addr > max_addr - 1):
        print("WARNING: Memory address larger than 24 bits was truncated")
        addr = addr % max_addr
    try:
        return MEM[addr]
    except KeyError:
        return 0

def print_sim_state():
    global PC
    print(" ze {:08X} {:<12} a1 {:08X} {:<12} t2 {:08X} {:<12} k0 {:08X} {:<12}".format(
            REGS[0], reg_contents(0), REGS[4], reg_contents(4),
            REGS[8], reg_contents(8), REGS[12], reg_contents(12)))
    print(" at {:08X} {:<12} a2 {:08X} {:<12} s0 {:08X} {:<12} sp {:08X} {:<12}".format(
            REGS[1], reg_contents(1), REGS[5], reg_contents(5),
            REGS[9], reg_contents(9), REGS[13], reg_contents(13)))
    print(" v0 {:08X} {:<12} t0 {:08X} {:<12} s1 {:08X} {:<12} fp {:08X} {:<12}".format(
            REGS[2], reg_contents(2), REGS[6], reg_contents(6),
            REGS[10], reg_contents(10), REGS[14], reg_contents(14)))
    print(" a0 {:08X} {:<12} t1 {:08X} {:<12} s2 {:08X} {:<12} ra {:08X} {:<12}".format(
            REGS[3], reg_contents(3), REGS[7], reg_contents(7),
            REGS[11], reg_contents(11), REGS[15], reg_contents(15)))
    #CCstr = ""
    #for flag, flagval in CC.items():
    #    if flagval:
    #        CCstr += flag
    #print(" - Condition Codes: {}".format(CCstr))
    print(" - Now at x{:08X}: {}".format(PC, print_instruction()))

def reg_contents(i):
    try:
        return '{:.11}'.format(LABELS[REGS[i]])
    except:
        return '{}'.format(force_signed(REGS[i], 16))

def bit(instr, bit):
    return instr[16 - bit]

def bit_range(instr, high, low):
    # b/c we're using strings, math is reversed order
    return instr[15 - high:15 - low + 1]

def print_instruction():
    global PC
    instruction = access_mem(PC)
    IREG = format(instruction, '016b')
    op = bit_range(IREG, 15, 13)

    if IREG == '0000000000000000':
        return 'noop'

    opname = OP_NAME[op]
    if opname in R_TYPES:            # ADD, NAND
        rx = bit_range(IREG, 12, 9)
        ry = bit_range(IREG, 8, 5)
        rz = bit_range(IREG, 4, 1)
        return '{} {}, {}, {}'.format(opname, REG_NAME[rx], REG_NAME[ry], REG_NAME[rz])
    elif opname in I_TYPES:           # ADDI, LW, SW, BEQ
        rx = bit_range(IREG, 12, 9)
        ry = bit_range(IREG, 8, 5)
        immval = bit_range(IREG, 4, 0)
        return '{} {}, {}, #{}'.format(opname, REG_NAME[rx], REG_NAME[ry], int2c(immval))
    elif opname in J_TYPES:           # JALR TODO CHECK THIS ONE FOR RA AND AT ORDER
        at = bit_range(IREG, 12, 9)
        ra = bit_range(IREG, 8, 5)
        return '{} {}, {}'.format(opname, REG_NAME[at], REG_NAME[ra])
    elif opname in O_TYPES:
        return opname
    else:
        return "Unrecognized instruction"

def print_help():
    print(""" Welcome to the simulator text interface. The available commands are:

    r[un] or c[ontinue]             resume execution until next breakpoint
    s[tep]                          execute one instruction
    b[reak] <addr or label>         view and set breakpoints
                                    ex) 'break'
                                    ex) 'break 0x3'
                                    ex) 'break main'
    print <lowaddr>-<highaddr>      print the values in memory between <lowaddr> and <highaddr>
                                    ex) 'print 0x20-0x30'
    q[uit]                          exit the simulator
    """)

def run():
    global HALTED
    while HALTED == False:
        step_instruction()
        if PC in BREAKPOINTS:
            break

def step_instruction():
    global PC
    instruction = access_mem(PC)
    PC += 1
    # decode instruction
    IREG = format(instruction, '016b')

    op = bit_range(IREG, 15,13)
#OP_NAME = {'000':'ADD', '001':'NAND', '010':'ADDI', '011':'LW',
#           '100':'SW', '101':'BEQ', '110':'JALR', '111':'HALT'}
    if OP_NAME[op] == 'ADD':
        dr_regno = uint(bit_range(IREG, 12, 9))
        sr1_regno = uint(bit_range(IREG, 8, 5))
        sr2_regno = uint(bit_range(IREG, 4, 1))
        num = force_signed(REGS[sr1_regno], 16) + force_signed(REGS[sr2_regno], 16)
        res = force_unsigned(num, 16)
        REGS[dr_regno] = res
    elif OP_NAME[op] == 'NAND':
        dr_regno = uint(bit_range(IREG, 12, 9))
        sr1_regno = uint(bit_range(IREG, 8, 5))
        sr2_regno = uint(bit_range(IREG, 4, 1))
        num = ~(REGS[sr1_regno] & REGS[sr2_regno])
        res = force_unsigned(num, 16)
        REGS[dr_regno] = res
    elif OP_NAME[op] == 'ADDI':
        dr_regno = uint(bit_range(IREG, 12, 9))
        sr1_regno = uint(bit_range(IREG, 8, 5))
        immval_int = int2c(bit_range(IREG, 4, 0))
        num = force_signed(REGS[sr1_regno], 16) + immval_int
        res = force_unsigned(num, 16)
        REGS[dr_regno] = res
    elif OP_NAME[op] == 'BEQ':
        rx = uint(bit_range(IREG, 12, 9))
        ry = uint(bit_range(IREG, 8, 5))
        offset_int = int2c(bit_range(IREG, 4, 0))
        if (REGS[rx] == REGS[ry]):
            PC = force_unsigned(PC + offset_int, 16)
    elif OP_NAME[op] == 'LW':
        dr_regno = uint(bit_range(IREG, 12, 9))
        base_regno = uint(bit_range(IREG, 8, 5))
        offset_int = int2c(bit_range(IREG, 4, 0))
        res = access_mem(force_unsigned(REGS[base_regno] + offset_int, 16))
        REGS[dr_regno] = res
    elif OP_NAME[op] == 'SW':
        sr_regno = uint(bit_range(IREG, 12, 9))
        base_regno = uint(bit_range(IREG, 8, 5))
        offset_int = int2c(bit_range(IREG, 4, 0))
        MEM[force_unsigned(REGS[base_regno] + offset_int, 16)] = REGS[sr_regno]
    elif OP_NAME[op] == 'JALR':
        at_regno = uint(bit_range(IREG, 12, 9))
        ra_regno = uint(bit_range(IREG, 8, 5))
        REGS[ra_regno] = PC
        PC = REGS[at_regno]
    elif OP_NAME[op] == 'HALT':
        # undo last pcinc
        PC -= 1
        global HALTED
        HALTED = True

    # make sure zero register remains 0
    REGS[0] = 0

    if PC in BREAKPOINTS:
        print("Breakpoint {} reached: PC = 0x{:X}".format(BREAKPOINTS.index(PC), PC))

def add_breakpoint(bp):
    try:
        bp = int(bp, 0)
    except ValueError:
        if bp in LABELS.values():
            # get PC val key by label value
            bp = LABELS.keys()[LABELS.values().index(bp)]
    if bp not in BREAKPOINTS:
        BREAKPOINTS.append(bp)
    print("[sim] Added breakpoint: PC = 0x{:X}".format(bp))

def print_breakpoints():
    if len(BREAKPOINTS) > 0:
        for i, bp in enumerate(BREAKPOINTS):
            print("[sim] Breakpoint {}: PC = 0x{:X}".format(i, bp))
    else:
        print("[sim] No breakpoints currently enabled")

def print_mem(lowaddr, highaddr):
    for i in xrange(lowaddr, highaddr+1):
        print("[0x{:X}]: 0x{:08X}".format(i, access_mem(i)))
    print("")

def prompt_input():
    global LAST_CMD
    break_regex = '(break|b)\s*(.+)?'
    mem_regex = 'print (\w+)-(\w+)'

    input_command = input("(sim) ")

    # allow repeating commands by pressing enter GDB-style
    if input_command == '':
        input_command = LAST_CMD
    else:
        LAST_CMD = input_command

    if input_command == 'help':
        print_help()
    elif input_command in ['run', 'r']:
        run()
    elif input_command in ['step', 's']:
        step_instruction()
    elif input_command in ['continue', 'c']:
        run()
    elif input_command in ['quit', 'q']:
        global QUIT
        QUIT = True
    elif re.match(break_regex, input_command):
        try:
            m = re.match(break_regex, input_command)
            if m.group(2):
                bp = m.group(2)
                add_breakpoint(bp)
            print_breakpoints()
        except:
            print("[sim] Error parsing requested breakpoint: {}".format(input_command))
    elif re.match(mem_regex, input_command):
        try:
            m = re.match(mem_regex, input_command)
            lowaddr = int(m.group(1), 0)
            highaddr = int(m.group(2), 0)
            print_mem(lowaddr, highaddr)
        except:
            print("[sim] Error parsing requested print: {}".format(input_command))
    else:
        print("[sim] Unknown command: {}".format(input_command))
        print("[sim] For help on commands and usage, use the 'help' command")

def run_sim():
    global QUIT
    while QUIT == False:
        try:
            print_sim_state()
            prompt_input()
        except KeyboardInterrupt:
            print("[sim] Simulation interrupted.")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('runfile', help=".bin file to be run")
    args = parser.parse_args()
    load_program(args.runfile)
    run_sim()
