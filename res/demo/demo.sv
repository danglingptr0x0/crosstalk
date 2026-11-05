module demo(
    input logic clk,
    input logic rst,
    output logic [7:0] led,
    output logic [8:0] serial_out,
    output logic [7:0] exc_cause,
    output logic osc
);
    localparam logic [3:0] OP_CPY = 4'h0;
    localparam logic [3:0] OP_LD = 4'h1;
    localparam logic [3:0] OP_ST = 4'h2;
    localparam logic [3:0] OP_ADD = 4'h3;
    localparam logic [3:0] OP_SUB = 4'h4;
    localparam logic [3:0] OP_AND = 4'h5;
    localparam logic [3:0] OP_OR = 4'h6;
    localparam logic [3:0] OP_XOR = 4'h7;
    localparam logic [3:0] OP_SH = 4'h8;
    localparam logic [3:0] OP_JMP = 4'hC;

    localparam logic [2:0] COND_EQ = 3'h0;
    localparam logic [2:0] COND_NE = 3'h1;
    localparam logic [2:0] COND_LT = 3'h2;
    localparam logic [2:0] COND_GE = 3'h3;
    localparam logic [2:0] COND_LTU = 3'h4;
    localparam logic [2:0] COND_GEU = 3'h5;
    localparam logic [2:0] COND_ALWAYS = 3'h6;

    localparam logic [7:0] EXC_NONE = 8'h00;
    localparam logic [7:0] EXC_INVALID = 8'h01;
    localparam logic [7:0] EXC_UNALIGNED = 8'h02;
    localparam logic [7:0] EXC_RESERVED_REG = 8'h06;

    localparam logic [63:0] MEM_END = 64'h0000000000001000;
    localparam logic [51:0] VMEM_PAGE = 52'h00000000000B8;
    localparam logic [63:0] IO_SERIAL = 64'h0000000090000000;
    localparam logic [63:0] IO_LED = 64'h0000000090000008;
    localparam logic [63:0] INSTR_BYTES = 64'h0000000000000008;

    logic [63:0] mem [0:511];
    logic [7:0] vmem [0:4095];

    logic [63:0] dr0;
    logic [63:0] dr1;
    logic [63:0] dr2;
    logic [63:0] dr3;
    logic [63:0] dr4;
    logic [63:0] dr5;
    logic [63:0] dr6;
    logic [63:0] dr7;
    logic [63:0] dr8;
    logic [63:0] dr9;
    logic [63:0] dr10;
    logic [63:0] dr11;
    logic [63:0] dr12;
    logic [63:0] dr13;
    logic [63:0] dr14;
    logic [63:0] dr15;

    logic [63:0] pc;
    logic [63:0] loc;
    logic [63:0] exec_instr;
    logic [63:0] fetch_instr;
    logic exec_valid;
    logic hlt;
    logic [2:0] osc_div;

    logic [3:0] op;
    logic [5:0] dst;
    logic [5:0] src;
    logic f0;
    logic f1;
    logic f2;
    logic f3;
    logic [1:0] width;
    logic [2:0] cond;
    logic [31:0] imm;
    logic [63:0] imm64;
    logic [63:0] off64;
    logic [63:0] lhs;
    logic [63:0] rhs;
    logic [63:0] addr;
    logic [63:0] mem_word;
    logic [5:0] lane_shift;
    logic [63:0] lane;
    logic [63:0] wmask;
    logic [63:0] ld_val;
    logic [63:0] res;
    logic wr_en;
    logic take;
    logic lt_s;
    logic [7:0] exc;
    logic in_mem;
    logic in_vmem;
    logic aligned;

    assign fetch_instr = mem[pc[11:3]];
    assign osc = osc_div[2];

    assign op = exec_instr[63:60];
    assign dst = exec_instr[59:54];
    assign src = exec_instr[53:48];
    assign f0 = exec_instr[47];
    assign f1 = exec_instr[46];
    assign f2 = exec_instr[45];
    assign f3 = exec_instr[44];
    assign width = exec_instr[47:46];
    assign cond = exec_instr[47:45];
    assign imm = exec_instr[31:0];
    assign imm64 = {32'h00000000, imm};
    assign off64 = {{32{imm[31]}}, imm};

    always_comb begin
        case (dst)
            6'd0: lhs = dr0;
            6'd1: lhs = dr1;
            6'd2: lhs = dr2;
            6'd3: lhs = dr3;
            6'd4: lhs = dr4;
            6'd5: lhs = dr5;
            6'd6: lhs = dr6;
            6'd7: lhs = dr7;
            6'd8: lhs = dr8;
            6'd9: lhs = dr9;
            6'd10: lhs = dr10;
            6'd11: lhs = dr11;
            6'd12: lhs = dr12;
            6'd13: lhs = dr13;
            6'd14: lhs = dr14;
            6'd15: lhs = dr15;
            default: lhs = 64'h0;
        endcase
    end

    always_comb begin
        case (src)
            6'd0: rhs = dr0;
            6'd1: rhs = dr1;
            6'd2: rhs = dr2;
            6'd3: rhs = dr3;
            6'd4: rhs = dr4;
            6'd5: rhs = dr5;
            6'd6: rhs = dr6;
            6'd7: rhs = dr7;
            6'd8: rhs = dr8;
            6'd9: rhs = dr9;
            6'd10: rhs = dr10;
            6'd11: rhs = dr11;
            6'd12: rhs = dr12;
            6'd13: rhs = dr13;
            6'd14: rhs = dr14;
            6'd15: rhs = dr15;
            default: rhs = 64'h0;
        endcase
    end

    always_comb begin
        addr = (op == OP_ST) ? lhs + imm64 : rhs + imm64;
        mem_word = mem[addr[11:3]];
        lane_shift = {addr[2:0], 3'b000};
        lane = mem_word >> lane_shift;
        in_mem = addr < MEM_END;
        in_vmem = addr[63:12] == VMEM_PAGE;

        case (width)
            2'd0: wmask = 64'h00000000000000FF;
            2'd1: wmask = 64'h000000000000FFFF;
            2'd2: wmask = 64'h00000000FFFFFFFF;
            default: wmask = 64'hFFFFFFFFFFFFFFFF;
        endcase

        case (width)
            2'd0: aligned = 1'b1;
            2'd1: aligned = addr[0] == 1'b0;
            2'd2: aligned = addr[1:0] == 2'b00;
            default: aligned = addr[2:0] == 3'b000;
        endcase

        if (in_vmem) begin
            ld_val = {56'h0, vmem[addr[11:0]]};
        end else if (addr == IO_LED) begin
            ld_val = {56'h0, led};
        end else begin
            ld_val = lane & wmask;
        end
    end

    always_comb begin
        if (lhs[63] != rhs[63]) begin
            lt_s = lhs[63];
        end else begin
            lt_s = lhs < rhs;
        end

        case (cond)
            COND_EQ: take = lhs == rhs;
            COND_NE: take = lhs != rhs;
            COND_LT: take = lt_s;
            COND_GE: take = ~lt_s;
            COND_LTU: take = lhs < rhs;
            COND_GEU: take = lhs >= rhs;
            COND_ALWAYS: take = 1'b1;
            default: take = 1'b0;
        endcase
    end

    always_comb begin
        res = 64'h0;
        wr_en = 1'b0;
        exc = EXC_NONE;

        case (op)
            OP_CPY: begin
                res = f0 ? imm64 : rhs;
                wr_en = 1'b1;
                if (f1) begin
                    exc = EXC_INVALID;
                end
            end
            OP_LD: begin
                res = ld_val;
                wr_en = 1'b1;
                if (f2) begin
                    exc = EXC_INVALID;
                end else if (!aligned) begin
                    exc = EXC_UNALIGNED;
                end
            end
            OP_ST: begin
                if (!aligned) begin
                    exc = EXC_UNALIGNED;
                end
            end
            OP_ADD: begin
                res = lhs + (f1 ? imm64 : rhs);
                wr_en = 1'b1;
            end
            OP_SUB: begin
                res = lhs - (f1 ? imm64 : rhs);
                wr_en = 1'b1;
            end
            OP_AND: begin
                res = lhs & rhs;
                wr_en = 1'b1;
            end
            OP_OR: begin
                res = lhs | rhs;
                wr_en = 1'b1;
            end
            OP_XOR: begin
                res = lhs ^ rhs;
                wr_en = 1'b1;
            end
            OP_SH: begin
                if (f2) begin
                    res = f0 ? lhs >> imm[5:0] : lhs << imm[5:0];
                end else begin
                    res = f0 ? lhs >> rhs[5:0] : lhs << rhs[5:0];
                end
                wr_en = 1'b1;
                if (f1) begin
                    exc = EXC_INVALID;
                end
            end
            OP_JMP: begin
                if (!f3 || cond == 3'h7) begin
                    exc = EXC_INVALID;
                end
            end
            default: begin
                exc = EXC_INVALID;
            end
        endcase

        if (wr_en && dst > 6'd15 && exc == EXC_NONE) begin
            exc = EXC_RESERVED_REG;
        end
    end

    initial begin
        mem[0] = 64'h0040800000000400;
        mem[1] = 64'h02008000000B8148;
        mem[2] = 64'h00808000000B8148;
        mem[3] = 64'h0100800090000000;
        mem[4] = 64'h024080000000000A;
        mem[5] = 64'h028080000000000A;
        mem[6] = 64'h02C080000000000D;
        mem[7] = 64'h03C0800000001000;
        mem[8] = 64'h10C1000000000000;
        mem[9] = 64'hC0C0100000000068;
        mem[10] = 64'h3040400000000001;
        mem[11] = 64'hC0C9100000000030;
        mem[12] = 64'h2103000000000000;
        mem[13] = 64'h2083000000000000;
        mem[14] = 64'h208A000000000001;
        mem[15] = 64'h3080400000000002;
        mem[16] = 64'hC000D000FFFFFFC0;
        mem[17] = 64'h210B000000000000;
        mem[18] = 64'h2103000000000000;
        mem[19] = 64'h32004000000000A0;
        mem[20] = 64'h0088000000000000;
        mem[21] = 64'hC000D000FFFFFF98;
        mem[22] = 64'h0140800000000001;
        mem[23] = 64'h2105000000000008;
        mem[24] = 64'h43C0400000000008;
        mem[25] = 64'h23C5C00000000000;
        mem[26] = 64'h0180800000010000;
        mem[27] = 64'h4180400000000001;
        mem[28] = 64'hC1803000FFFFFFF8;
        mem[29] = 64'h8140200000000001;
        mem[30] = 64'h01C0800000000100;
        mem[31] = 64'hC1473000FFFFFFC0;
        mem[32] = 64'h0140800000000001;
        mem[33] = 64'h03C0800000001000;
        mem[34] = 64'hC000D000FFFFFFA8;
        mem[128] = 64'h20656D6F636C6557;
        mem[129] = 64'h73736F7243206F74;
        mem[130] = 64'h68540A2E6B6C6174;
        mem[131] = 64'h20756F79206B6E61;
        mem[132] = 64'h6E776F6420726F66;
        mem[133] = 64'h2E676E6964616F6C;
        mem[134] = 64'h746E6F63206F540A;
        mem[135] = 64'h202C657475626972;
        mem[136] = 64'h4E4F432064616572;
        mem[137] = 64'h4E49545542495254;
        mem[138] = 64'h00000000000A2E47;
    end

    always_ff @(posedge clk) begin
        osc_div <= osc_div + 3'h1;

        if (rst) begin
            pc <= 64'h0;
            loc <= 64'h0;
            exec_instr <= 64'h0;
            exec_valid <= 1'b0;
            hlt <= 1'b0;
            exc_cause <= EXC_NONE;
            led <= 8'h00;
            serial_out <= 9'h000;
            dr1 <= 64'h0;
            dr2 <= 64'h0;
            dr3 <= 64'h0;
            dr4 <= 64'h0;
            dr5 <= 64'h0;
            dr6 <= 64'h0;
            dr7 <= 64'h0;
            dr8 <= 64'h0;
            dr9 <= 64'h0;
            dr10 <= 64'h0;
            dr11 <= 64'h0;
            dr12 <= 64'h0;
            dr13 <= 64'h0;
            dr14 <= 64'h0;
            dr15 <= 64'h0;
        end else if (!hlt) begin
            exec_instr <= fetch_instr;
            loc <= pc;
            exec_valid <= 1'b1;
            pc <= pc + INSTR_BYTES;

            if (exec_valid) begin
                if (exc != EXC_NONE) begin
                    exc_cause <= exc;
                    hlt <= 1'b1;
                    exec_instr <= exec_instr;
                    loc <= loc;
                    pc <= pc;
                end else begin
                    if (op == OP_JMP && take) begin
                        pc <= loc + off64;
                        exec_valid <= 1'b0;
                    end

                    if (op == OP_ST) begin
                        if (in_vmem) begin
                            vmem[addr[11:0]] <= rhs[7:0];
                        end else if (addr == IO_SERIAL) begin
                            serial_out <= {~serial_out[8], rhs[7:0]};
                        end else if (addr == IO_LED) begin
                            led <= rhs[7:0];
                        end else if (in_mem) begin
                            mem[addr[11:3]] <= (mem_word & ~(wmask << lane_shift)) | ((rhs & wmask) << lane_shift);
                        end
                    end

                    if (wr_en) begin
                        case (dst)
                            6'd1: dr1 <= res;
                            6'd2: dr2 <= res;
                            6'd3: dr3 <= res;
                            6'd4: dr4 <= res;
                            6'd5: dr5 <= res;
                            6'd6: dr6 <= res;
                            6'd7: dr7 <= res;
                            6'd8: dr8 <= res;
                            6'd9: dr9 <= res;
                            6'd10: dr10 <= res;
                            6'd11: dr11 <= res;
                            6'd12: dr12 <= res;
                            6'd13: dr13 <= res;
                            6'd14: dr14 <= res;
                            6'd15: dr15 <= res;
                            default: dr0 <= 64'h0;
                        endcase
                    end
                end
            end
        end
    end
endmodule
