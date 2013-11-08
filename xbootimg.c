/****************************************************************************************
[BOOT|RECOVERY|DROIDBOOT].UNSIGNED Structure    
[DV | PV] STITCH                        
+==============+=============+============+=============+==================+========+
|        START |         END |       SIZE |  Total Size |             FLAG |        |
+==============+=============+============+=============+==================+========+
|  0x0000 0000 |             |            |             |          CMDLINE |  ZONE1 |
|              | 0x0000 03FF | 1024 Bytes |             |   + Zero Padding |        |
+--------------+-------------+------------+             |------------------+--------+
|  0x0000 0400 | 0x0000 0403 |    4 Bytes |             |      Kernel Size |        |
|  0x0000 0404 | 0x0000 0407 |    4 Bytes |   4 K Bytes |     Ramdisk Size |        |
|  0x0000 0408 | 0x0000 040B |    4 Bytes |             |               $5 |        |
|  0x0000 040C | 0x0000 040F |    4 Bytes |             |               $6 |  ZONE2 |
+--------------+-------------+------------+             |------------------+        |
|  0x0000 0410 |             |            |             |                  |        |
|              | 0x0000 0FFF | 3056 Bytes |             |   + Zero Padding |        |
+==============+=============+============+=============+==================+========+
|  0x0000 1000 |             |                          |         BOOTSTUB |  ZONE3 |
|              | 0x0000 1FFF |         4 K Bytes        |   + Zero Padding |        |
+==============+=============+==========================+==================+========+
|  0x0000 2000 |             |                          |           KERNEL |  ZONE4 |
|              | 0xXXXX XXXX |        XXXX Bytes        |        + RAMDISK |        |
+==============+=============+==========================+==================+========+

****************************************************************************************

SIGNED_[BOOT|RECOVERY|DROIDBOOT].[BIN|IMG] Structure??   
[PV] ISU                        
+==============+=============+============+=============+==================+
|        START |         END |       SIZE |  Total Size |             FLAG |
+==============+=============+============+=============+==================+
|  0x0000 0000 |             |                          |                  |
|              | 0x0000 01E0 |         480 Bytes        |       ISU HEADER |
+==============+=============+==========================+==================+
|              |             |                          |                  |
|              |             |                          |     xxx.UNSIGNED |
|              |             |                          |               +  |
|              |             |                          |   "0xFF" PADDING |
|              |             |                          |                  |
+==============+=============+==========================+==================+

                     
[BOOT|RECOVERY|DROIDBOOT].[BIN|IMG] Structure??  
[PV] ISU + XFSTK-STITCHER                        
+==============+=============+============+=============+==================+
|        START |         END |       SIZE |  Total Size |             FLAG |
+==============+=============+============+=============+==================+
|  0x0000 0000 |             |                          |                  |
|              | 0x0000 01FF |         512 Bytes        |XFSTK HEADER(55AA)|
+==============+=============+==========================+==================+
|              |             |                          |                  |
|              |             |                          |       SIGNED_xxx |
|              |             |                          |               +  |
|              |             |                          |   "0xFF" PADDING |
|              |             |                          |                  |
+==============+=============+==========================+==================+

****************************************************************************************

SIGNED_[BOOT|RECOVERY|DROIDBOOT].[BIN|IMG] Structure??   
[DV] XFSTK-STITCHER                        
+==============+=============+============+=============+==================+
|        START |         END |       SIZE |  Total Size |             FLAG |
+==============+=============+============+=============+==================+
|  0x0000 0000 |             |                          |                  |
|              | 0x0000 01FF |         512 Bytes        |XFSTK HEADER(55AA)|
+==============+=============+==========================+==================+
|              |             |                          |                  |
|              |             |                          |     xxx.UNSIGNED |
|              |             |                          |                + |
|              |             |                          |   "0xFF" PADDING |
|              |             |                          |                  |
+==============+=============+==========================+==================+
****************************************************************************************/
                 
#define DESCRIPTION "XBOOTIMG - Unpack Intel Soc Images(xxx.unsigned and PV/DV xxx.img)"      
#define VERSION     "V_0_1"
#define AUTHOR      "shenpengru@gmail.com"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ZONE1_SIZE_CMDLINE      (1024)
#define ZONE2_SIZE_PAR          (1024*3)
#define ZONE3_SIZE_BOOTSTUB     (1024*4)

#define SIZE_OF_ZONE2_PAR       (4)
#define POSITION_KERNEL_SIZE    (0x00000400)
#define POSITION_RAMDISK_SIZE   (0x00000404)
#define POSITION_PAR_5          (0x00000408)
#define POSITION_PAR_6          (0x0000040c)

#define POSITION_CMDLINE        (0x00000000)
#define POSITION_BOOTSTUB       (0x00001000)
#define POSITION_KERNELRAMDISK  (0x00002000)

#define FILE_CMDLINE            "cmdline"
#define FILE_BOOTSTUB           "bootstub"
#define FILE_KERNEL             "kernel"
#define FILE_RAMDISK            "ramdisk.img"
#define FILE_PAR5               "par5"
#define FILE_PAR6               "par6"

#define DEBUG (0)
#if DEBUG
#define INFO printf
#else
#define INFO //
#endif

extern char DATE[];

void help(void)
{
	printf("\n\tVersion    : %s\n", VERSION );
	printf("\tDescrip    : %s\n", DESCRIPTION);
	printf("\tAuthor     : %s\n", AUTHOR );
	printf("\tBuild Time : %s\n", DATE);
	printf("\n\tUsage:\n" );
	printf("\t\t[unsigned image] xbootimg image_file_name\n" );
	printf("\t\t[DV][boot image] xbootimg image_file_name -d\n" );
	printf("\t\t[PV][boot image] xbootimg image_file_name -p\n\n" );
}

int main( int argc, char** argv )
{
	FILE* img_fd;
	FILE* cmd_fd, *stub_fd, *kernel_fd, *ramdisk_fd, *par5_fd, *par6_fd;
	int i;
	unsigned int  buffer, buf[4], buff[4];
	unsigned int size_kernel, size_ramdisk;

	unsigned int offset = 0;

	if ( argc < 2 ){
		help();
		exit( 1 );
	}

	if( argc == 3 ){
		if( strcmp(argv[2], "-d") == 0 ){
			offset = 512;
		}
		else if ( strcmp(argv[2], "-p") == 0 ){
			offset = 512+480;
		}
		else{
			printf("ERROR: Wrong input parameter!\n");
			help();
			exit(1);	
		}
	}
	else{
		offset = 0;
	}

	INFO("offset = %d\n", offset);

	img_fd = fopen( argv[1], "r" );
	if ( img_fd == NULL ){
		printf( "\tOpen image file [%s] failed\n", argv[1] );
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
	 * Get kernel size
	 */
	INFO("\n===== READ KERNEL/RAMDISK SIZE =====\n");
	fseek( img_fd, POSITION_KERNEL_SIZE + offset, SEEK_SET );
	for( i=0; i<SIZE_OF_ZONE2_PAR; i++ ){
		fread( &buf[i], 1, 1, img_fd );
	}
	size_kernel = ((buf[3]&0xFF)*256*256*256)+((buf[2]&0xFF)*256*256)+((buf[1]&0xFF)*256)+(buf[0]&0xFF);
	printf("Kernel  Size %d KBytes\n", size_kernel/1024);
	
	/*
	 * Get ramdisk size
	 */
	fseek( img_fd, POSITION_RAMDISK_SIZE + offset, SEEK_SET );
	for( i=0; i<SIZE_OF_ZONE2_PAR; i++ ){
		fread( &buf[i], 1, 1, img_fd );
	}
	size_ramdisk = ((buf[3]&0xFF)*256*256*256)+((buf[2]&0xFF)*256*256)+((buf[1]&0xFF)*256)+(buf[0]&0xFF);
	printf("Ramdisk Size %d KBytes\n", size_ramdisk/1024);

	/*
	 * Get kernel
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
	 * Get ramdisk
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

