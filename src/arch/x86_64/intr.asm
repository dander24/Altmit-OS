global irq0_handler
global irq1_handler
global irq2_handler
global irq3_handler
global irq4_handler
global irq5_handler
global irq6_handler
global irq7_handler
global irq8_handler
global irq9_handler
global irq10_handler
global irq11_handler
global irq12_handler
global irq13_handler
global irq14_handler
global irq15_handler
global irq16_handler
global irq17_handler
global irq18_handler
global irq19_handler
global irq20_handler
global irq21_handler
global irq22_handler
global irq23_handler
global irq24_handler
global irq25_handler
global irq26_handler
global irq27_handler
global irq28_handler
global irq29_handler
global irq30_handler
global irq31_handler
global irq32_handler
global irq33_handler
global irq34_handler
global irq35_handler
global irq36_handler
global irq37_handler
global irq38_handler
global irq39_handler
global irq40_handler
global irq41_handler
global irq42_handler
global irq43_handler
global irq44_handler
global irq45_handler
global irq46_handler
global irq47_handler
global irq48_handler
global irq49_handler
global irq50_handler
global irq51_handler
global irq52_handler
global irq53_handler
global irq54_handler
global irq55_handler
global irq56_handler
global irq57_handler
global irq58_handler
global irq59_handler
global irq60_handler
global irq61_handler
global irq62_handler
global irq63_handler
global irq64_handler
global irq65_handler
global irq66_handler
global irq67_handler
global irq68_handler
global irq69_handler
global irq70_handler
global irq71_handler
global irq72_handler
global irq73_handler
global irq74_handler
global irq75_handler
global irq76_handler
global irq77_handler
global irq78_handler
global irq79_handler
global irq80_handler
global irq81_handler
global irq82_handler
global irq83_handler
global irq84_handler
global irq85_handler
global irq86_handler
global irq87_handler
global irq88_handler
global irq89_handler
global irq90_handler
global irq91_handler
global irq92_handler
global irq93_handler
global irq94_handler
global irq95_handler
global irq96_handler
global irq97_handler
global irq98_handler
global irq99_handler
global irq100_handler
global irq101_handler
global irq102_handler
global irq103_handler
global irq104_handler
global irq105_handler
global irq106_handler
global irq107_handler
global irq108_handler
global irq109_handler
global irq110_handler
global irq111_handler
global irq112_handler
global irq113_handler
global irq114_handler
global irq115_handler
global irq116_handler
global irq117_handler
global irq118_handler
global irq119_handler
global irq120_handler
global irq121_handler
global irq122_handler
global irq123_handler
global irq124_handler
global irq125_handler
global irq126_handler
global irq127_handler
global irq128_handler
global irq129_handler
global irq130_handler
global irq131_handler
global irq132_handler
global irq133_handler
global irq134_handler
global irq135_handler
global irq136_handler
global irq137_handler
global irq138_handler
global irq139_handler
global irq140_handler
global irq141_handler
global irq142_handler
global irq143_handler
global irq144_handler
global irq145_handler
global irq146_handler
global irq147_handler
global irq148_handler
global irq149_handler
global irq150_handler
global irq151_handler
global irq152_handler
global irq153_handler
global irq154_handler
global irq155_handler
global irq156_handler
global irq157_handler
global irq158_handler
global irq159_handler
global irq160_handler
global irq161_handler
global irq162_handler
global irq163_handler
global irq164_handler
global irq165_handler
global irq166_handler
global irq167_handler
global irq168_handler
global irq169_handler
global irq170_handler
global irq171_handler
global irq172_handler
global irq173_handler
global irq174_handler
global irq175_handler
global irq176_handler
global irq177_handler
global irq178_handler
global irq179_handler
global irq180_handler
global irq181_handler
global irq182_handler
global irq183_handler
global irq184_handler
global irq185_handler
global irq186_handler
global irq187_handler
global irq188_handler
global irq189_handler
global irq190_handler
global irq191_handler
global irq192_handler
global irq193_handler
global irq194_handler
global irq195_handler
global irq196_handler
global irq197_handler
global irq198_handler
global irq199_handler
global irq200_handler
global irq201_handler
global irq202_handler
global irq203_handler
global irq204_handler
global irq205_handler
global irq206_handler
global irq207_handler
global irq208_handler
global irq209_handler
global irq210_handler
global irq211_handler
global irq212_handler
global irq213_handler
global irq214_handler
global irq215_handler
global irq216_handler
global irq217_handler
global irq218_handler
global irq219_handler
global irq220_handler
global irq221_handler
global irq222_handler
global irq223_handler
global irq224_handler
global irq225_handler
global irq226_handler
global irq227_handler
global irq228_handler
global irq229_handler
global irq230_handler
global irq231_handler
global irq232_handler
global irq233_handler
global irq234_handler
global irq235_handler
global irq236_handler
global irq237_handler
global irq238_handler
global irq239_handler
global irq240_handler
global irq241_handler
global irq242_handler
global irq243_handler
global irq244_handler
global irq245_handler
global irq246_handler
global irq247_handler
global irq248_handler
global irq249_handler
global irq250_handler
global irq251_handler
global irq252_handler
global irq253_handler
global irq254_handler
global irq255_handler

global common_irq_handler
extern irq_c_handler

common_irq_handler:
	push rsi
error_irq_handler:
	push rax
	push rbx
	push rcx
	push rdx	
	push rsp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	call irq_c_handler
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsp
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rsi
	pop rdi
	iretq

irq0_handler:
	push rdi
	mov rdi, 0
	jmp common_irq_handler

irq1_handler:
	push rdi
	mov rdi, 1
	jmp common_irq_handler

irq2_handler:
	push rdi
	mov rdi, 2
	jmp common_irq_handler

irq3_handler:
	push rdi
	mov rdi, 3
	jmp common_irq_handler

irq4_handler:
	push rdi
	mov rdi, 4
	jmp common_irq_handler

irq5_handler:
	push rdi
	mov rdi, 5
	jmp common_irq_handler

irq6_handler:
	push rdi
	mov rdi, 6
	jmp common_irq_handler

irq7_handler:
	push rdi
	mov rdi, 7
	jmp common_irq_handler

irq8_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 8
	jmp error_irq_handler

irq9_handler:
	push rdi
	mov rdi, 9
	jmp common_irq_handler

irq10_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 10
	jmp error_irq_handler

irq11_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 11
	jmp error_irq_handler

irq12_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 12
	jmp error_irq_handler

irq13_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 13
	jmp error_irq_handler

irq14_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 14
	jmp error_irq_handler

irq15_handler:
	push rdi
	mov rdi, 15
	jmp common_irq_handler

irq16_handler:
	push rdi
	mov rdi, 16
	jmp common_irq_handler

irq17_handler:
	push rsi
	mov rsi, [rsp+8]
	mov [rsp+8], rdi
	mov rdi, 17
	jmp error_irq_handler

irq18_handler:
	push rdi
	mov rdi, 18
	jmp common_irq_handler

irq19_handler:
	push rdi
	mov rdi, 19
	jmp common_irq_handler

irq20_handler:
	push rdi
	mov rdi, 20
	jmp common_irq_handler

irq21_handler:
	push rdi
	mov rdi, 21
	jmp common_irq_handler

irq22_handler:
	push rdi
	mov rdi, 22
	jmp common_irq_handler

irq23_handler:
	push rdi
	mov rdi, 23
	jmp common_irq_handler

irq24_handler:
	push rdi
	mov rdi, 24
	jmp common_irq_handler

irq25_handler:
	push rdi
	mov rdi, 25
	jmp common_irq_handler

irq26_handler:
	push rdi
	mov rdi, 26
	jmp common_irq_handler

irq27_handler:
	push rdi
	mov rdi, 27
	jmp common_irq_handler

irq28_handler:
	push rdi
	mov rdi, 28
	jmp common_irq_handler

irq29_handler:
	push rdi
	mov rdi, 29
	jmp common_irq_handler

irq30_handler:
	push rdi
	mov rdi, 30
	jmp common_irq_handler

irq31_handler:
	push rdi
	mov rdi, 31
	jmp common_irq_handler

irq32_handler:
	push rdi
	mov rdi, 32
	jmp common_irq_handler

irq33_handler:
	push rdi
	mov rdi, 33
	jmp common_irq_handler

irq34_handler:
	push rdi
	mov rdi, 34
	jmp common_irq_handler

irq35_handler:
	push rdi
	mov rdi, 35
	jmp common_irq_handler

irq36_handler:
	push rdi
	mov rdi, 36
	jmp common_irq_handler

irq37_handler:
	push rdi
	mov rdi, 37
	jmp common_irq_handler

irq38_handler:
	push rdi
	mov rdi, 38
	jmp common_irq_handler

irq39_handler:
	push rdi
	mov rdi, 39
	jmp common_irq_handler

irq40_handler:
	push rdi
	mov rdi, 40
	jmp common_irq_handler

irq41_handler:
	push rdi
	mov rdi, 41
	jmp common_irq_handler

irq42_handler:
	push rdi
	mov rdi, 42
	jmp common_irq_handler

irq43_handler:
	push rdi
	mov rdi, 43
	jmp common_irq_handler

irq44_handler:
	push rdi
	mov rdi, 44
	jmp common_irq_handler

irq45_handler:
	push rdi
	mov rdi, 45
	jmp common_irq_handler

irq46_handler:
	push rdi
	mov rdi, 46
	jmp common_irq_handler

irq47_handler:
	push rdi
	mov rdi, 47
	jmp common_irq_handler

irq48_handler:
	push rdi
	mov rdi, 48
	jmp common_irq_handler

irq49_handler:
	push rdi
	mov rdi, 49
	jmp common_irq_handler

irq50_handler:
	push rdi
	mov rdi, 50
	jmp common_irq_handler

irq51_handler:
	push rdi
	mov rdi, 51
	jmp common_irq_handler

irq52_handler:
	push rdi
	mov rdi, 52
	jmp common_irq_handler

irq53_handler:
	push rdi
	mov rdi, 53
	jmp common_irq_handler

irq54_handler:
	push rdi
	mov rdi, 54
	jmp common_irq_handler

irq55_handler:
	push rdi
	mov rdi, 55
	jmp common_irq_handler

irq56_handler:
	push rdi
	mov rdi, 56
	jmp common_irq_handler

irq57_handler:
	push rdi
	mov rdi, 57
	jmp common_irq_handler

irq58_handler:
	push rdi
	mov rdi, 58
	jmp common_irq_handler

irq59_handler:
	push rdi
	mov rdi, 59
	jmp common_irq_handler

irq60_handler:
	push rdi
	mov rdi, 60
	jmp common_irq_handler

irq61_handler:
	push rdi
	mov rdi, 61
	jmp common_irq_handler

irq62_handler:
	push rdi
	mov rdi, 62
	jmp common_irq_handler

irq63_handler:
	push rdi
	mov rdi, 63
	jmp common_irq_handler

irq64_handler:
	push rdi
	mov rdi, 64
	jmp common_irq_handler

irq65_handler:
	push rdi
	mov rdi, 65
	jmp common_irq_handler

irq66_handler:
	push rdi
	mov rdi, 66
	jmp common_irq_handler

irq67_handler:
	push rdi
	mov rdi, 67
	jmp common_irq_handler

irq68_handler:
	push rdi
	mov rdi, 68
	jmp common_irq_handler

irq69_handler:
	push rdi
	mov rdi, 69
	jmp common_irq_handler

irq70_handler:
	push rdi
	mov rdi, 70
	jmp common_irq_handler

irq71_handler:
	push rdi
	mov rdi, 71
	jmp common_irq_handler

irq72_handler:
	push rdi
	mov rdi, 72
	jmp common_irq_handler

irq73_handler:
	push rdi
	mov rdi, 73
	jmp common_irq_handler

irq74_handler:
	push rdi
	mov rdi, 74
	jmp common_irq_handler

irq75_handler:
	push rdi
	mov rdi, 75
	jmp common_irq_handler

irq76_handler:
	push rdi
	mov rdi, 76
	jmp common_irq_handler

irq77_handler:
	push rdi
	mov rdi, 77
	jmp common_irq_handler

irq78_handler:
	push rdi
	mov rdi, 78
	jmp common_irq_handler

irq79_handler:
	push rdi
	mov rdi, 79
	jmp common_irq_handler

irq80_handler:
	push rdi
	mov rdi, 80
	jmp common_irq_handler

irq81_handler:
	push rdi
	mov rdi, 81
	jmp common_irq_handler

irq82_handler:
	push rdi
	mov rdi, 82
	jmp common_irq_handler

irq83_handler:
	push rdi
	mov rdi, 83
	jmp common_irq_handler

irq84_handler:
	push rdi
	mov rdi, 84
	jmp common_irq_handler

irq85_handler:
	push rdi
	mov rdi, 85
	jmp common_irq_handler

irq86_handler:
	push rdi
	mov rdi, 86
	jmp common_irq_handler

irq87_handler:
	push rdi
	mov rdi, 87
	jmp common_irq_handler

irq88_handler:
	push rdi
	mov rdi, 88
	jmp common_irq_handler

irq89_handler:
	push rdi
	mov rdi, 89
	jmp common_irq_handler

irq90_handler:
	push rdi
	mov rdi, 90
	jmp common_irq_handler

irq91_handler:
	push rdi
	mov rdi, 91
	jmp common_irq_handler

irq92_handler:
	push rdi
	mov rdi, 92
	jmp common_irq_handler

irq93_handler:
	push rdi
	mov rdi, 93
	jmp common_irq_handler

irq94_handler:
	push rdi
	mov rdi, 94
	jmp common_irq_handler

irq95_handler:
	push rdi
	mov rdi, 95
	jmp common_irq_handler

irq96_handler:
	push rdi
	mov rdi, 96
	jmp common_irq_handler

irq97_handler:
	push rdi
	mov rdi, 97
	jmp common_irq_handler

irq98_handler:
	push rdi
	mov rdi, 98
	jmp common_irq_handler

irq99_handler:
	push rdi
	mov rdi, 99
	jmp common_irq_handler

irq100_handler:
	push rdi
	mov rdi, 100
	jmp common_irq_handler

irq101_handler:
	push rdi
	mov rdi, 101
	jmp common_irq_handler

irq102_handler:
	push rdi
	mov rdi, 102
	jmp common_irq_handler

irq103_handler:
	push rdi
	mov rdi, 103
	jmp common_irq_handler

irq104_handler:
	push rdi
	mov rdi, 104
	jmp common_irq_handler

irq105_handler:
	push rdi
	mov rdi, 105
	jmp common_irq_handler

irq106_handler:
	push rdi
	mov rdi, 106
	jmp common_irq_handler

irq107_handler:
	push rdi
	mov rdi, 107
	jmp common_irq_handler

irq108_handler:
	push rdi
	mov rdi, 108
	jmp common_irq_handler

irq109_handler:
	push rdi
	mov rdi, 109
	jmp common_irq_handler

irq110_handler:
	push rdi
	mov rdi, 110
	jmp common_irq_handler

irq111_handler:
	push rdi
	mov rdi, 111
	jmp common_irq_handler

irq112_handler:
	push rdi
	mov rdi, 112
	jmp common_irq_handler

irq113_handler:
	push rdi
	mov rdi, 113
	jmp common_irq_handler

irq114_handler:
	push rdi
	mov rdi, 114
	jmp common_irq_handler

irq115_handler:
	push rdi
	mov rdi, 115
	jmp common_irq_handler

irq116_handler:
	push rdi
	mov rdi, 116
	jmp common_irq_handler

irq117_handler:
	push rdi
	mov rdi, 117
	jmp common_irq_handler

irq118_handler:
	push rdi
	mov rdi, 118
	jmp common_irq_handler

irq119_handler:
	push rdi
	mov rdi, 119
	jmp common_irq_handler

irq120_handler:
	push rdi
	mov rdi, 120
	jmp common_irq_handler

irq121_handler:
	push rdi
	mov rdi, 121
	jmp common_irq_handler

irq122_handler:
	push rdi
	mov rdi, 122
	jmp common_irq_handler

irq123_handler:
	push rdi
	mov rdi, 123
	jmp common_irq_handler

irq124_handler:
	push rdi
	mov rdi, 124
	jmp common_irq_handler

irq125_handler:
	push rdi
	mov rdi, 125
	jmp common_irq_handler

irq126_handler:
	push rdi
	mov rdi, 126
	jmp common_irq_handler

irq127_handler:
	push rdi
	mov rdi, 127
	jmp common_irq_handler

irq128_handler:
	push rdi
	mov rdi, 128
	jmp common_irq_handler

irq129_handler:
	push rdi
	mov rdi, 129
	jmp common_irq_handler

irq130_handler:
	push rdi
	mov rdi, 130
	jmp common_irq_handler

irq131_handler:
	push rdi
	mov rdi, 131
	jmp common_irq_handler

irq132_handler:
	push rdi
	mov rdi, 132
	jmp common_irq_handler

irq133_handler:
	push rdi
	mov rdi, 133
	jmp common_irq_handler

irq134_handler:
	push rdi
	mov rdi, 134
	jmp common_irq_handler

irq135_handler:
	push rdi
	mov rdi, 135
	jmp common_irq_handler

irq136_handler:
	push rdi
	mov rdi, 136
	jmp common_irq_handler

irq137_handler:
	push rdi
	mov rdi, 137
	jmp common_irq_handler

irq138_handler:
	push rdi
	mov rdi, 138
	jmp common_irq_handler

irq139_handler:
	push rdi
	mov rdi, 139
	jmp common_irq_handler

irq140_handler:
	push rdi
	mov rdi, 140
	jmp common_irq_handler

irq141_handler:
	push rdi
	mov rdi, 141
	jmp common_irq_handler

irq142_handler:
	push rdi
	mov rdi, 142
	jmp common_irq_handler

irq143_handler:
	push rdi
	mov rdi, 143
	jmp common_irq_handler

irq144_handler:
	push rdi
	mov rdi, 144
	jmp common_irq_handler

irq145_handler:
	push rdi
	mov rdi, 145
	jmp common_irq_handler

irq146_handler:
	push rdi
	mov rdi, 146
	jmp common_irq_handler

irq147_handler:
	push rdi
	mov rdi, 147
	jmp common_irq_handler

irq148_handler:
	push rdi
	mov rdi, 148
	jmp common_irq_handler

irq149_handler:
	push rdi
	mov rdi, 149
	jmp common_irq_handler

irq150_handler:
	push rdi
	mov rdi, 150
	jmp common_irq_handler

irq151_handler:
	push rdi
	mov rdi, 151
	jmp common_irq_handler

irq152_handler:
	push rdi
	mov rdi, 152
	jmp common_irq_handler

irq153_handler:
	push rdi
	mov rdi, 153
	jmp common_irq_handler

irq154_handler:
	push rdi
	mov rdi, 154
	jmp common_irq_handler

irq155_handler:
	push rdi
	mov rdi, 155
	jmp common_irq_handler

irq156_handler:
	push rdi
	mov rdi, 156
	jmp common_irq_handler

irq157_handler:
	push rdi
	mov rdi, 157
	jmp common_irq_handler

irq158_handler:
	push rdi
	mov rdi, 158
	jmp common_irq_handler

irq159_handler:
	push rdi
	mov rdi, 159
	jmp common_irq_handler

irq160_handler:
	push rdi
	mov rdi, 160
	jmp common_irq_handler

irq161_handler:
	push rdi
	mov rdi, 161
	jmp common_irq_handler

irq162_handler:
	push rdi
	mov rdi, 162
	jmp common_irq_handler

irq163_handler:
	push rdi
	mov rdi, 163
	jmp common_irq_handler

irq164_handler:
	push rdi
	mov rdi, 164
	jmp common_irq_handler

irq165_handler:
	push rdi
	mov rdi, 165
	jmp common_irq_handler

irq166_handler:
	push rdi
	mov rdi, 166
	jmp common_irq_handler

irq167_handler:
	push rdi
	mov rdi, 167
	jmp common_irq_handler

irq168_handler:
	push rdi
	mov rdi, 168
	jmp common_irq_handler

irq169_handler:
	push rdi
	mov rdi, 169
	jmp common_irq_handler

irq170_handler:
	push rdi
	mov rdi, 170
	jmp common_irq_handler

irq171_handler:
	push rdi
	mov rdi, 171
	jmp common_irq_handler

irq172_handler:
	push rdi
	mov rdi, 172
	jmp common_irq_handler

irq173_handler:
	push rdi
	mov rdi, 173
	jmp common_irq_handler

irq174_handler:
	push rdi
	mov rdi, 174
	jmp common_irq_handler

irq175_handler:
	push rdi
	mov rdi, 175
	jmp common_irq_handler

irq176_handler:
	push rdi
	mov rdi, 176
	jmp common_irq_handler

irq177_handler:
	push rdi
	mov rdi, 177
	jmp common_irq_handler

irq178_handler:
	push rdi
	mov rdi, 178
	jmp common_irq_handler

irq179_handler:
	push rdi
	mov rdi, 179
	jmp common_irq_handler

irq180_handler:
	push rdi
	mov rdi, 180
	jmp common_irq_handler

irq181_handler:
	push rdi
	mov rdi, 181
	jmp common_irq_handler

irq182_handler:
	push rdi
	mov rdi, 182
	jmp common_irq_handler

irq183_handler:
	push rdi
	mov rdi, 183
	jmp common_irq_handler

irq184_handler:
	push rdi
	mov rdi, 184
	jmp common_irq_handler

irq185_handler:
	push rdi
	mov rdi, 185
	jmp common_irq_handler

irq186_handler:
	push rdi
	mov rdi, 186
	jmp common_irq_handler

irq187_handler:
	push rdi
	mov rdi, 187
	jmp common_irq_handler

irq188_handler:
	push rdi
	mov rdi, 188
	jmp common_irq_handler

irq189_handler:
	push rdi
	mov rdi, 189
	jmp common_irq_handler

irq190_handler:
	push rdi
	mov rdi, 190
	jmp common_irq_handler

irq191_handler:
	push rdi
	mov rdi, 191
	jmp common_irq_handler

irq192_handler:
	push rdi
	mov rdi, 192
	jmp common_irq_handler

irq193_handler:
	push rdi
	mov rdi, 193
	jmp common_irq_handler

irq194_handler:
	push rdi
	mov rdi, 194
	jmp common_irq_handler

irq195_handler:
	push rdi
	mov rdi, 195
	jmp common_irq_handler

irq196_handler:
	push rdi
	mov rdi, 196
	jmp common_irq_handler

irq197_handler:
	push rdi
	mov rdi, 197
	jmp common_irq_handler

irq198_handler:
	push rdi
	mov rdi, 198
	jmp common_irq_handler

irq199_handler:
	push rdi
	mov rdi, 199
	jmp common_irq_handler

irq200_handler:
	push rdi
	mov rdi, 200
	jmp common_irq_handler

irq201_handler:
	push rdi
	mov rdi, 201
	jmp common_irq_handler

irq202_handler:
	push rdi
	mov rdi, 202
	jmp common_irq_handler

irq203_handler:
	push rdi
	mov rdi, 203
	jmp common_irq_handler

irq204_handler:
	push rdi
	mov rdi, 204
	jmp common_irq_handler

irq205_handler:
	push rdi
	mov rdi, 205
	jmp common_irq_handler

irq206_handler:
	push rdi
	mov rdi, 206
	jmp common_irq_handler

irq207_handler:
	push rdi
	mov rdi, 207
	jmp common_irq_handler

irq208_handler:
	push rdi
	mov rdi, 208
	jmp common_irq_handler

irq209_handler:
	push rdi
	mov rdi, 209
	jmp common_irq_handler

irq210_handler:
	push rdi
	mov rdi, 210
	jmp common_irq_handler

irq211_handler:
	push rdi
	mov rdi, 211
	jmp common_irq_handler

irq212_handler:
	push rdi
	mov rdi, 212
	jmp common_irq_handler

irq213_handler:
	push rdi
	mov rdi, 213
	jmp common_irq_handler

irq214_handler:
	push rdi
	mov rdi, 214
	jmp common_irq_handler

irq215_handler:
	push rdi
	mov rdi, 215
	jmp common_irq_handler

irq216_handler:
	push rdi
	mov rdi, 216
	jmp common_irq_handler

irq217_handler:
	push rdi
	mov rdi, 217
	jmp common_irq_handler

irq218_handler:
	push rdi
	mov rdi, 218
	jmp common_irq_handler

irq219_handler:
	push rdi
	mov rdi, 219
	jmp common_irq_handler

irq220_handler:
	push rdi
	mov rdi, 220
	jmp common_irq_handler

irq221_handler:
	push rdi
	mov rdi, 221
	jmp common_irq_handler

irq222_handler:
	push rdi
	mov rdi, 222
	jmp common_irq_handler

irq223_handler:
	push rdi
	mov rdi, 223
	jmp common_irq_handler

irq224_handler:
	push rdi
	mov rdi, 224
	jmp common_irq_handler

irq225_handler:
	push rdi
	mov rdi, 225
	jmp common_irq_handler

irq226_handler:
	push rdi
	mov rdi, 226
	jmp common_irq_handler

irq227_handler:
	push rdi
	mov rdi, 227
	jmp common_irq_handler

irq228_handler:
	push rdi
	mov rdi, 228
	jmp common_irq_handler

irq229_handler:
	push rdi
	mov rdi, 229
	jmp common_irq_handler

irq230_handler:
	push rdi
	mov rdi, 230
	jmp common_irq_handler

irq231_handler:
	push rdi
	mov rdi, 231
	jmp common_irq_handler

irq232_handler:
	push rdi
	mov rdi, 232
	jmp common_irq_handler

irq233_handler:
	push rdi
	mov rdi, 233
	jmp common_irq_handler

irq234_handler:
	push rdi
	mov rdi, 234
	jmp common_irq_handler

irq235_handler:
	push rdi
	mov rdi, 235
	jmp common_irq_handler

irq236_handler:
	push rdi
	mov rdi, 236
	jmp common_irq_handler

irq237_handler:
	push rdi
	mov rdi, 237
	jmp common_irq_handler

irq238_handler:
	push rdi
	mov rdi, 238
	jmp common_irq_handler

irq239_handler:
	push rdi
	mov rdi, 239
	jmp common_irq_handler

irq240_handler:
	push rdi
	mov rdi, 240
	jmp common_irq_handler

irq241_handler:
	push rdi
	mov rdi, 241
	jmp common_irq_handler

irq242_handler:
	push rdi
	mov rdi, 242
	jmp common_irq_handler

irq243_handler:
	push rdi
	mov rdi, 243
	jmp common_irq_handler

irq244_handler:
	push rdi
	mov rdi, 244
	jmp common_irq_handler

irq245_handler:
	push rdi
	mov rdi, 245
	jmp common_irq_handler

irq246_handler:
	push rdi
	mov rdi, 246
	jmp common_irq_handler

irq247_handler:
	push rdi
	mov rdi, 247
	jmp common_irq_handler

irq248_handler:
	push rdi
	mov rdi, 248
	jmp common_irq_handler

irq249_handler:
	push rdi
	mov rdi, 249
	jmp common_irq_handler

irq250_handler:
	push rdi
	mov rdi, 250
	jmp common_irq_handler

irq251_handler:
	push rdi
	mov rdi, 251
	jmp common_irq_handler

irq252_handler:
	push rdi
	mov rdi, 252
	jmp common_irq_handler

irq253_handler:
	push rdi
	mov rdi, 253
	jmp common_irq_handler

irq254_handler:
	push rdi
	mov rdi, 254
	jmp common_irq_handler

irq255_handler:
	push rdi
	mov rdi, 255
	jmp common_irq_handler



