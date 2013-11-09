/* Header Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

/*************************************** Version Info ************************************* */ 
#define DESCRIPTION "xbootimg - Unpack Intel Soc Images(xxx.unsigned and PV/DV xxx.img)"      
#define VERSION     "alpha 0.2.0"
#define AUTHOR      "shenpengru@gmail.com"
extern char DATE[];
/*************************************** Version Info **************************************/                 

/* ZONE Size */
#define ZONE1_SIZE_CMDLINE      (1024)
#define ZONE2_SIZE_PAR          (1024*3)
#define ZONE3_SIZE_BOOTSTUB     (1024*4)

/* Positions */
#define SIZE_OF_ZONE2_PAR       (4)
#define POSITION_KERNEL_SIZE    (0x00000400)
#define POSITION_RAMDISK_SIZE   (0x00000404)
#define POSITION_PAR_5          (0x00000408)
#define POSITION_PAR_6          (0x0000040c)

#define POSITION_CMDLINE        (0x00000000)
#define POSITION_BOOTSTUB       (0x00001000)
#define POSITION_KERNELRAMDISK  (0x00002000)

/* Output fils' name */
#define FILE_CMDLINE            "cmdline"
#define FILE_BOOTSTUB           "bootstub"
#define FILE_KERNEL             "kernel"
#define FILE_RAMDISK            "ramdisk.img"
#define FILE_PAR5               "par5"
#define FILE_PAR6               "par6"

/* Length of header */
#define OFFSET_ISU_HEADER	(480)
#define OFFSET_XFSTK_HEADER	(512)

enum ETYPE{
	ENUM_UNSIGNED_IMAGE=0,
	ENUM_SIGNEDONLY_IMAGE=1,	// isu
	ENUM_DV_IMAGE=2,		// xfstk
	ENUM_PV_IMAGE=3,		// isu+xfstk
};

#define DEBUG (0)
#if DEBUG
#define INFO printf
#else
#define INFO //
#endif

#define SHOW_VERSION	(1)
#define HIDE_VERSION	(0)
void help(int version)
{
	if(version){
		printf("\n\t========== XBOOTIMAGE INTEL SOC ==========\n\n");
		printf("\tDescription : %s\n", DESCRIPTION);
		printf("\tVersion     : %s\n", VERSION );
		printf("\tAuthor      : %s\n", AUTHOR );
		printf("\tBuild Time  : %s\n", DATE);
		printf("\n\t==========================================\n\n");
	}
	else{
		printf("\n\tUsage:\n" );
		printf("\t\txbootimg --image <image_name> [--type <pv|dv|signed>]\n");
		printf("\t\txbootimg --version\n");
		printf("\n\tExample:\n" );
		printf("\t\txbootimg --image boot.unsigned\n" );
		printf("\t\txbootimg --image signed_boot.img --type signed\n" );
		printf("\t\txbootimg --image boot.bin --type pv\n" );
		printf("\t\txbootimg --image boot.bin --type dv\n\n" );
	}
}

int check_xfstk_header(char* image_name)
{
	FILE* image_fd;
	unsigned int flag;

	image_fd = fopen( image_name, "r" );
	if( image_fd == NULL ){
		printf("%s: open image %s fail!\n", __func__, image_name);
	}

	fseek( image_fd, 512-2, SEEK_SET );
	fread( &flag, 2, 1, image_fd );
	INFO("FLAG: %x %x\n", (flag)&0xFF, (flag>>8)&0xFF );
	if ( (flag&0xFF)==0x55 && ((flag>>8)&0xFF)==0xAA ){
		fclose( image_fd );
		return 0;
	}
	else{
		printf("This is not a xfstk stitched image. Please check your image's type.\n");
		fclose( image_fd );
		return -1;
	}
}

int main( int argc, char** argv )
{
	FILE* img_fd;
	FILE* cmd_fd, *stub_fd, *kernel_fd, *ramdisk_fd, *par5_fd, *par6_fd;
	int i;
	unsigned int  buffer, buf[4], buff[4];
	unsigned int size_kernel, size_ramdisk;
	char image[50];
	enum ETYPE type;

	unsigned int offset = 0;

	int opt;
	int option_index = 0;
	char *optstring = ":h:";
	static struct option long_options[] = {  
		{  "image", required_argument, NULL, 'i'},  
		{   "type", required_argument, NULL, 't'},
		{   "help", no_argument,       NULL, 'h'},
		{"version", no_argument,       NULL, 'v'},
		{0, 0, 0, 0}  
	};

	if ( argc < 2 ){
		printf("\n\tERROR: Too few argruments!\n");
		help(HIDE_VERSION);
		exit( 1 );
	}

	while( (opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1 ){
		switch( opt ){
			case 'i':
				INFO("Input image is %s\n", optarg);
				strcpy(image, optarg);
				break;
			case 't':
				if ( strcmp( optarg, "signed" )==0 )
					type = ENUM_SIGNEDONLY_IMAGE;
				else if ( strcmp( optarg, "pv" )==0 )
					type = ENUM_PV_IMAGE;
				else if ( strcmp( optarg, "dv" )==0 )
					type = ENUM_DV_IMAGE;
				else {
					printf("\n\tERROR: Unknown type!\n");
					exit(1);
				}
				break;
			case 'h':
				help(HIDE_VERSION);
				exit(0);
				break;
			case 'v':
				help(SHOW_VERSION);
				exit(0);
			case '?':
			default:
				printf("\n\tERROR: Unknown options!\n");
				exit(1);
				break;
		}
	}

	offset = 0;
	switch( type ){
		case ENUM_SIGNEDONLY_IMAGE:
			offset = OFFSET_ISU_HEADER;
			printf("");
			break;
		case ENUM_DV_IMAGE:
			offset = OFFSET_XFSTK_HEADER;
			if( check_xfstk_header( image ) != 0 ) exit(1);
			break;
		case ENUM_PV_IMAGE:
			offset = OFFSET_ISU_HEADER + OFFSET_XFSTK_HEADER;
			if( check_xfstk_header( image ) != 0 ) exit(1);
			break;
		default:
			type = ENUM_UNSIGNED_IMAGE;
			offset = 0;
			break;
	}

	if ( offset == 0 ){
		type = ENUM_UNSIGNED_IMAGE;
	}
	printf("\nIMAGE TYPE: %d(0:unsigned/1:signed/2:DV/3:PV), HEADER LENGTH : %d\n", type, offset);

	img_fd = fopen( image, "r" );
	if ( img_fd == NULL ){
		printf( "\tOpen image file [%s] failed\n", image );
		exit( 1 );
	}

	/*
	 * Get cmdline
	 */
	INFO("\n===== CMDLINE =====\n");
	fseek( img_fd, POSITION_CMDLINE + offset, SEEK_SET );
	cmd_fd = fopen( FILE_CMDLINE, "wb" );
	for( i=0; i<ZONE1_SIZE_CMDLINE; i++ ){
		fread( &buffer, 1, 1, img_fd );
		fwrite( &buffer, 1, 1, cmd_fd );
	}
	fclose(cmd_fd);

	/*
	 * Get bootstub
	 */
	INFO("\n===== BOOTSTUB =====\n");
	fseek( img_fd, POSITION_BOOTSTUB + offset, SEEK_SET );
	stub_fd = fopen( FILE_BOOTSTUB, "wb" );
	for( i=0; i<ZONE3_SIZE_BOOTSTUB; i++ ){
		fread( &buffer, 1, 1, img_fd );
		fwrite( &buffer, 1, 1, stub_fd );
	}
	fclose(stub_fd);

	/*
	 * Get kernel and ramdisk size
	 */
	INFO("\n===== READ KERNEL/RAMDISK SIZE =====\n");
	fseek( img_fd, POSITION_KERNEL_SIZE + offset, SEEK_SET );
	for( i=0; i<SIZE_OF_ZONE2_PAR; i++ ){
		fread( &buf[i], 1, 1, img_fd );
	}
	size_kernel = ((buf[3]&0xFF)*256*256*256)+((buf[2]&0xFF)*256*256)+((buf[1]&0xFF)*256)+(buf[0]&0xFF);
	fseek( img_fd, POSITION_RAMDISK_SIZE + offset, SEEK_SET );
	for( i=0; i<SIZE_OF_ZONE2_PAR; i++ ){
		fread( &buf[i], 1, 1, img_fd );
	}
	size_ramdisk = ((buf[3]&0xFF)*256*256*256)+((buf[2]&0xFF)*256*256)+((buf[1]&0xFF)*256)+(buf[0]&0xFF);
	printf("\nKernel Size %d KBytes, Ramdisk Size %d KBytes\n\n", size_kernel/1024, size_ramdisk/1024);

	/*
	 * Get kernel image
	 */
	INFO("\n===== READ KERNEL =====\n");
	fseek( img_fd, POSITION_KERNELRAMDISK + offset, SEEK_SET );
	kernel_fd = fopen( FILE_KERNEL, "wb" );
	for( i=0; i<size_kernel; i++ ){
		fread( &buffer, 1, 1, img_fd );
		fwrite( &buffer, 1, 1, kernel_fd );
	}
	fclose( kernel_fd );

	/*
	 * Get ramdisk image
	 */
	INFO("\n===== READ RAMDISK =====\n");
	fseek( img_fd, POSITION_KERNELRAMDISK+size_kernel + offset, SEEK_SET );
	ramdisk_fd = fopen( FILE_RAMDISK, "wb" );
	for( i=0; i<size_ramdisk; i++ ){
		fread( &buffer, 1, 1, img_fd );
		fwrite( &buffer, 1, 1, ramdisk_fd );
	}
	fclose( ramdisk_fd );

	fclose( img_fd );
	return 0;
}

