#define FAKEFLASH_START 0x9a000000
#define FAKEFLASH_END   0x9f200000

#define FLASH_START 0x9fc00000
#define FLASH_END   0x9fe00000
#define FLASH_SIZE  0x00200000

#define RAM_START   0x80000000
#define RAM_END     0x82000000
#define RAM_SIZE    0x02000000

#define REG_START   0xfffe0000
#define REG_END     0xffffffff

struct cpu_state;

struct callback
{
	struct callback* next;
	uint32_t address;
	void(*callback)(struct cpu_state *);
};

struct cpu_state
{
	int32_t reg[32];
	int32_t HI;
	int32_t LO;
	int32_t pc;
	int32_t prev_pc[3];
	int32_t delayed_jump;
	int32_t jump_pc;
	int32_t eret;
	bool in_irq;
	struct callback *callbacks;
	int8_t *ram;
	int8_t *flash;
	int32_t cop0[32][10];
};

extern struct cpu_state cpu;
extern bool debug;
extern bool run;
extern bool step;

void initialize_emulator(int8_t** ram, int8_t** flash);
void initialize_cpu(struct cpu_state *cpu, int8_t* ram, int8_t* flash, int32_t start_address);
void register_callbacks(void);
void register_callback(struct cpu_state *cpu, uint32_t address, void(*callback)(struct cpu_state *));
void bp(struct cpu_state *cpu);
void instlog(struct cpu_state *cpu);
void execute(struct cpu_state *cpu);

void print_string(struct cpu_state *cpu);
void printf_string(struct cpu_state *cpu);
void print_char(struct cpu_state *cpu);

