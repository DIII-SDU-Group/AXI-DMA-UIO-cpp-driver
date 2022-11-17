#include "axi_dma_controller.h"


// void print_mem(void *virtual_address, int byte_count)
// {
// 	char *data_ptr = (char *)virtual_address;

// 	for(int i=0;i<byte_count;i++){
// 		printf("%02X", data_ptr[i]);

// 		// print a space every 4 bytes (0 indexed)
// 		if(i%4==3){
// 			printf(" ");
// 		}
// 	}

// 	printf("\n");
// }


// int main() {
// 	uint32_t src_address = 0x0e000000;
// 	uint32_t dst_address = 0x0f000000;


//     printf("Opening a character device file of the Arty's DDR memeory...\n");
// 	int ddr_memory = open("/dev/mem", O_RDWR | O_SYNC);

// 	printf("Memory map the MM2S source address register block.\n");
//     unsigned int *virtual_src_addr  = (unsigned int *)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, src_address);

// 	printf("Memory map the S2MM destination address register block.\n");
//     unsigned int *virtual_dst_addr = (unsigned int *)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, dst_address);

//     printf("Writing random data to source register block...\n");
// 	virtual_src_addr[0]= 0xEFBEADDE;
// 	virtual_src_addr[1]= 0x11223344;
// 	virtual_src_addr[2]= 0xABABABAB;
// 	virtual_src_addr[3]= 0xCDCDCDCD;
// 	virtual_src_addr[4]= 0x00001111;
// 	virtual_src_addr[5]= 0x22223333;
// 	virtual_src_addr[6]= 0x44445555;
// 	virtual_src_addr[7]= 0x66667777;

// 	printf("Clearing the destination register block...\n");
//     memset(virtual_dst_addr, 0, 32);

// 	printf("Source register block:\n");
// 	print_mem(virtual_src_addr, 32);

// 	printf("Destination register block:\n");
// 	print_mem(virtual_dst_addr, 32);






//     AXIDMAController dma(0, 0x10000);

// 	printf("Reset the DMA.\n");

// 	dma.MM2SReset();
// 	dma.S2MMReset();

// 	printf("Halting the DMA.\n");
// 	dma.MM2SHalt();
// 	dma.S2MMHalt();

// 	printf("Enable interrupts.\n");
// 	dma.MM2SInterruptEnable();
// 	dma.S2MMInterruptEnable();

// 	printf("Set the source address.\n");
// 	dma.MM2SSetSourceAddress(src_address);

// 	printf("Set the destination address.\n");
// 	dma.S2MMSetDestinationAddress(dst_address);

// 	printf("Set MM2S length.\n");
// 	dma.MM2SSetLength(32);

// 	printf("Set S2MM length.\n");
// 	dma.S2MMSetLength(32);

// 	printf("Run the MM2S channel.\n");
// 	dma.MM2SStart();

// 	printf("Run the S2MM channel.\n");
// 	dma.S2MMStart();

// 	printf("Wait for MM2S synchronization.\n");
// 	while(!dma.MM2SIsSynced());

// 	printf("Wait for S2MM synchronization.\n");
// 	while(!dma.S2MMIsSynced());

	// printf("Check MM2S status.\n");
	// DMAStatus mm2s_status = dma.MM2SGetStatus();
	// printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

	// printf("Check S2MM status.\n");
	// DMAStatus s2mm_status = dma.S2MMGetStatus();
	// printf("S2MM status: %s\n", s2mm_status.to_string().c_str());

// 	printf("Print the source data.\n");
// 	print_mem(virtual_src_addr, 32);

// 	printf("Print the destination data.\n");
// 	print_mem(virtual_dst_addr, 32);

// }






#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>

#define MM2S_CONTROL_REGISTER       0x00
#define MM2S_STATUS_REGISTER        0x04
#define MM2S_SRC_ADDRESS_REGISTER   0x18
#define MM2S_TRNSFR_LENGTH_REGISTER 0x28

#define S2MM_CONTROL_REGISTER       0x30
#define S2MM_STATUS_REGISTER        0x34
#define S2MM_DST_ADDRESS_REGISTER   0x48
#define S2MM_BUFF_LENGTH_REGISTER   0x58

#define IOC_IRQ_FLAG                1<<12
#define IDLE_FLAG                   1<<1

#define STATUS_HALTED               0x00000001
#define STATUS_IDLE                 0x00000002
#define STATUS_SG_INCLDED           0x00000008
#define STATUS_DMA_INTERNAL_ERR     0x00000010
#define STATUS_DMA_SLAVE_ERR        0x00000020
#define STATUS_DMA_DECODE_ERR       0x00000040
#define STATUS_SG_INTERNAL_ERR      0x00000100
#define STATUS_SG_SLAVE_ERR         0x00000200
#define STATUS_SG_DECODE_ERR        0x00000400
#define STATUS_IOC_IRQ              0x00001000
#define STATUS_DELAY_IRQ            0x00002000
#define STATUS_ERR_IRQ              0x00004000

#define HALT_DMA                    0x00000000
#define RUN_DMA                     0x00000001
#define RESET_DMA                   0x00000004
#define ENABLE_IOC_IRQ              0x00001000
#define ENABLE_DELAY_IRQ            0x00002000
#define ENABLE_ERR_IRQ              0x00004000
#define ENABLE_ALL_IRQ              0x00007000

unsigned int write_dma(unsigned int *virtual_addr, int offset, unsigned int value)
{
    virtual_addr[offset>>2] = value;

    return 0;
}

unsigned int read_dma(unsigned int *virtual_addr, int offset)
{
    return virtual_addr[offset>>2];
}

void dma_s2mm_status(unsigned int *virtual_addr)
{
    unsigned int status = read_dma(virtual_addr, S2MM_STATUS_REGISTER);

    printf("Stream to memory-mapped status (0x%08x@0x%02x):", status, S2MM_STATUS_REGISTER);

    if (status & STATUS_HALTED) {
		printf(" Halted.\n");
	} else {
		printf(" Running.\n");
	}

    if (status & STATUS_IDLE) {
		printf(" Idle.\n");
	}

    if (status & STATUS_SG_INCLDED) {
		printf(" SG is included.\n");
	}

    if (status & STATUS_DMA_INTERNAL_ERR) {
		printf(" DMA internal error.\n");
	}

    if (status & STATUS_DMA_SLAVE_ERR) {
		printf(" DMA slave error.\n");
	}

    if (status & STATUS_DMA_DECODE_ERR) {
		printf(" DMA decode error.\n");
	}

    if (status & STATUS_SG_INTERNAL_ERR) {
		printf(" SG internal error.\n");
	}

    if (status & STATUS_SG_SLAVE_ERR) {
		printf(" SG slave error.\n");
	}

    if (status & STATUS_SG_DECODE_ERR) {
		printf(" SG decode error.\n");
	}

    if (status & STATUS_IOC_IRQ) {
		printf(" IOC interrupt occurred.\n");
	}

    if (status & STATUS_DELAY_IRQ) {
		printf(" Interrupt on delay occurred.\n");
	}

    if (status & STATUS_ERR_IRQ) {
		printf(" Error interrupt occurred.\n");
	}
}

void dma_mm2s_status(unsigned int *virtual_addr)
{
    unsigned int status = read_dma(virtual_addr, MM2S_STATUS_REGISTER);

    printf("Memory-mapped to stream status (0x%08x@0x%02x):", status, MM2S_STATUS_REGISTER);

    if (status & STATUS_HALTED) {
		printf(" Halted.\n");
	} else {
		printf(" Running.\n");
	}

    if (status & STATUS_IDLE) {
		printf(" Idle.\n");
	}

    if (status & STATUS_SG_INCLDED) {
		printf(" SG is included.\n");
	}

    if (status & STATUS_DMA_INTERNAL_ERR) {
		printf(" DMA internal error.\n");
	}

    if (status & STATUS_DMA_SLAVE_ERR) {
		printf(" DMA slave error.\n");
	}

    if (status & STATUS_DMA_DECODE_ERR) {
		printf(" DMA decode error.\n");
	}

    if (status & STATUS_SG_INTERNAL_ERR) {
		printf(" SG internal error.\n");
	}

    if (status & STATUS_SG_SLAVE_ERR) {
		printf(" SG slave error.\n");
	}

    if (status & STATUS_SG_DECODE_ERR) {
		printf(" SG decode error.\n");
	}

    if (status & STATUS_IOC_IRQ) {
		printf(" IOC interrupt occurred.\n");
	}

    if (status & STATUS_DELAY_IRQ) {
		printf(" Interrupt on delay occurred.\n");
	}

    if (status & STATUS_ERR_IRQ) {
		printf(" Error interrupt occurred.\n");
	}
}

int dma_mm2s_sync(unsigned int *virtual_addr)
{
    unsigned int mm2s_status =  read_dma(virtual_addr, MM2S_STATUS_REGISTER);

	// sit in this while loop as long as the status does not read back 0x00001002 (4098)
	// 0x00001002 = IOC interrupt has occured and DMA is idle
	while(!(mm2s_status & IOC_IRQ_FLAG) || !(mm2s_status & IDLE_FLAG))
	{
        dma_s2mm_status(virtual_addr);
        dma_mm2s_status(virtual_addr);

        mm2s_status =  read_dma(virtual_addr, MM2S_STATUS_REGISTER);
    }

	return 0;
}

int dma_s2mm_sync(unsigned int *virtual_addr)
{
    unsigned int s2mm_status = read_dma(virtual_addr, S2MM_STATUS_REGISTER);

	// sit in this while loop as long as the status does not read back 0x00001002 (4098)
	// 0x00001002 = IOC interrupt has occured and DMA is idle
	while(!(s2mm_status & IOC_IRQ_FLAG) || !(s2mm_status & IDLE_FLAG))
	{
        dma_s2mm_status(virtual_addr);
        dma_mm2s_status(virtual_addr);

        s2mm_status = read_dma(virtual_addr, S2MM_STATUS_REGISTER);
    }

	return 0;
}

void print_mem(void *virtual_address, int byte_count)
{
	char *data_ptr = (char *)virtual_address;

	for(int i=0;i<byte_count;i++){
		printf("%02X", data_ptr[i]);

		// print a space every 4 bytes (0 indexed)
		if(i%4==3){
			printf(" ");
		}
	}

	printf("\n");
}

int main()
{
    printf("Hello World! - Running DMA transfer test application.\n");

	printf("Opening a character device file of the Arty's DDR memeory...\n");
	int ddr_memory = open("/dev/mem", O_RDWR | O_SYNC);
	//int uio_file = open("/dev/uio0", O_RDWR | O_SYNC);

	printf("Memory map the address of the DMA AXI IP via its AXI lite control interface register block.\n");
    //unsigned int *dma_virtual_addr = (unsigned int *)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, 0xA0020000);
	//unsigned int *dma_virtual_addr = (unsigned int *)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, uio_file, 0x0);
	AXIDMAController dma(0, 0x10000);
	unsigned int *dma_virtual_addr = dma.uio_map;

	printf("Memory map the MM2S source address register block.\n");
    unsigned int *virtual_src_addr  = (unsigned int *)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, 0x0e000000);

	printf("Memory map the S2MM destination address register block.\n");
    unsigned int *virtual_dst_addr = (unsigned int *)mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, 0x0f000000);

	printf("Writing random data to source register block...\n");
	virtual_src_addr[0]= 0xEFBEADDE;
	virtual_src_addr[1]= 0x11223344;
	virtual_src_addr[2]= 0xABABABAB;
	virtual_src_addr[3]= 0xCDCDCDCD;
	virtual_src_addr[4]= 0x00001111;
	virtual_src_addr[5]= 0x22223333;
	virtual_src_addr[6]= 0x44445555;
	virtual_src_addr[7]= 0x66667777;

	printf("Clearing the destination register block...\n");
    memset(virtual_dst_addr, 0, 32);

    printf("Source memory block data:      ");
	print_mem(virtual_src_addr, 32);

    printf("Destination memory block data: ");
	print_mem(virtual_dst_addr, 32);


	printf("\n");


    printf("Reset the DMA.\n");
    write_dma(dma_virtual_addr, S2MM_CONTROL_REGISTER, RESET_DMA);
    write_dma(dma_virtual_addr, MM2S_CONTROL_REGISTER, RESET_DMA);

	printf("Check MM2S status.\n");
	DMAStatus mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

	printf("Check S2MM status.\n");
	DMAStatus s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());





	printf("\n");







	printf("Halt the DMA.\n");
    write_dma(dma_virtual_addr, S2MM_CONTROL_REGISTER, HALT_DMA);
    write_dma(dma_virtual_addr, MM2S_CONTROL_REGISTER, HALT_DMA);

	printf("Check MM2S status.\n");
	 mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

	printf("Check S2MM status.\n");
	 s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());






	printf("\n");






	printf("Enable all interrupts.\n");
    //write_dma(dma_virtual_addr, S2MM_CONTROL_REGISTER, ENABLE_ALL_IRQ);
    //write_dma(dma_virtual_addr, MM2S_CONTROL_REGISTER, ENABLE_ALL_IRQ);
	dma.MM2SInterruptEnable();
	dma.S2MMInterruptEnable();

	printf("Check MM2S status.\n");
	 mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

	printf("Check S2MM status.\n");
	 s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());





	printf("\n");







    printf("Writing source address of the data from MM2S in DDR...\n");
	dma.MM2SSetSourceAddress(0x0e000000);

	printf("Check MM2S status.\n");
	 mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

    printf("Writing the destination address for the data from S2MM in DDR...\n");
	dma.S2MMSetDestinationAddress(0x0f000000);

	printf("Check S2MM status.\n");
	 s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());






	printf("\n");





	printf("Run the MM2S channel.\n");
	dma.MM2SStart();

	printf("Check MM2S status.\n");
	 mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

	printf("Run the S2MM channel.\n");
	dma.S2MMStart();

	printf("Check S2MM status.\n");
	 s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());





	printf("\n");








    printf("Writing MM2S transfer length of 32 bytes...\n");
	dma.MM2SSetLength(32);

	printf("Check MM2S status.\n");
	 mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

    printf("Writing S2MM transfer length of 32 bytes...\n");
	dma.S2MMSetLength(32);

	printf("Check S2MM status.\n");
	s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());


	printf("\n");


    printf("Waiting for MM2S synchronization...\n");

	while(!dma.MM2SIsSynced()) {
		printf("Not synced yet...\n");
	}

	printf("Check MM2S status.\n");
	mm2s_status = dma.MM2SGetStatus();
	printf("MM2S status: %s\n", mm2s_status.to_string().c_str());

    printf("Waiting for S2MM sychronization...\n");

	while(!dma.S2MMIsSynced()) {
		printf("Not synced yet...\n");
	}

	printf("Check S2MM status.\n");
	 s2mm_status = dma.S2MMGetStatus();
	printf("S2MM status: %s\n", s2mm_status.to_string().c_str());


 


	printf("\n");




    printf("Destination memory block: ");
	print_mem(virtual_dst_addr, 32);

	printf("\n");

    return 0;
}