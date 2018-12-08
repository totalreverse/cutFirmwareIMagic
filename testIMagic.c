#include <stdio.h>
#include <string.h>
#include <time.h>
#include <usb.h>
#include <sys/time.h>

#define IMAGIC_VID        0x3561 // TACX
#define IMAGIC_PID        0x1902 // T1902

int verbose =0;

unsigned char readBuf[256];
 
struct usb_device* findIMagic() {
    struct usb_bus* bus;
    struct usb_device* dev;
    
    usb_init();
    usb_set_debug(verbose ? 255:0);
    
    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus; bus = bus->next) {        
        for (dev = bus->devices; dev; dev = dev->next) {            
            if (dev->descriptor.idVendor == IMAGIC_VID && dev->descriptor.idProduct == IMAGIC_PID) {
                return dev;
            }
        }
    }
    return NULL;
}

            
void read_and_print_imagic(struct usb_dev_handle* udev, struct usb_endpoint_descriptor* readEp ,struct usb_endpoint_descriptor* writeEp ) {
    
    int myBreak = 0x80;
    int run = 0x0;
    
    
    struct timeval cancelTime;
    int oldc=0x0;    
    int first=1;

    while(1) {
        usleep(40000);
        
        struct timeval current;
        gettimeofday(&current, NULL);

        int rc = usb_bulk_read(udev, readEp->bEndpointAddress, readBuf, 64, 500);
        if(rc != 21) {
            printf("Read Err=%d\n",rc);
	    break;
	}
	
	if(first) {
            first = 0;
            uint32_t serial = (((uint32_t)readBuf[18])<<8)+readBuf[19];
            uint32_t manufatcoringYear = readBuf[17];
            uint32_t firmwareVersion = readBuf[20];
            printf("Manufatcoring year 20%02d, Serial-ID 0x%04x (%02d%06d), Firmware Version 0x%02x\n", 
                   manufatcoringYear, serial, manufatcoringYear, serial, firmwareVersion);
        }
	
        uint32_t wheel = readBuf[1] + (readBuf[2]<<8);
        uint32_t cad = readBuf[3];
        uint32_t hr = readBuf[4];
        uint32_t hundredth = readBuf[8] % 100;
        uint32_t seconds = (readBuf[6]<<8) + readBuf[7];
        uint32_t elapsedTime = seconds * 100 + hundredth;
        uint32_t brk = readBuf[9];
        uint32_t cadSens = readBuf[10];
        uint32_t steer = readBuf[12]; 
        
        printf(" %d %3d %3d %5d:%02d %02x %02x - ", wheel, cad, hr, seconds , hundredth, brk, steer); 
        for(int j=0;j< 21;j++) {
            printf("%02x ",readBuf[j]);
        }
        
        int c = readBuf[0]^0xf0;
        printf("[%s ", c & 0x1 ?  "H" : " ");
        printf("%s ", c & 0x80 ? "C" : " ");
        printf("%s ", c & 0x40 ? "U" : " ");
        printf("%s ", c & 0x20 ? "D" : " ");
        printf("%s ", c & 0x10 ? "E" : " ");
        printf("%s ", (c & 0x0e) != 0x0e ? "?P?" : "   ");
        printf("%s] ", cadSens & 0x10 ? "CAD" : "   ");
            
        if(c & 0x40)
            myBreak += 1;
        if(c & 0x20)
            myBreak -= 1;          

        if((c & 0x10) && !(oldc & 0x10)) {
            run = ~run;
            if(!run) {
                char wBuf[2] = { myBreak, 0x01 }; // Pause Command 0x1,0x4 (or both 0x5) works
                int wc = usb_bulk_write(udev, writeEp->bEndpointAddress, wBuf,2 , 500 );
                if(wc != 2) {
                        fprintf(stderr,"\n\nWrite Error\n");
                        break;
                }
            }
        }

        if(run) {
            char wBuf[2] = { myBreak, 0x0 };
            int wc = usb_bulk_write(udev, writeEp->bEndpointAddress, wBuf,2 , 500 );
            if(wc != 2) {
                fprintf(stderr,"\n\nWrite Error\n");
                break;
            }
            printf("Running: Break=%02x ",myBreak);
        } else {
            printf("Paused. Press OK  ");
        }
            
        if(c & 0x80 && !(oldc & 0x80)) {
            gettimeofday(&cancelTime, NULL);
            cancelTime.tv_sec += 3;
        } if(c & 0x80) {           
            int32_t delta = (cancelTime.tv_sec - current.tv_sec) *1000000 + cancelTime.tv_usec - current.tv_usec;
            if(delta < 0) {
                break;
            }
            printf("Stop in %1.2fs",delta/1000000.0f);
        }
        else {           
            printf("               ");
        }
        oldc = c;
        
        printf("\r");
    }
    printf("\n\n");
}

int main(int argc, char *argv[])
{
    char logfile[256];    
    
    if (argc > 1 && !strcmp(argv[1], "-v"))
        verbose = 1;
        
    struct usb_device* dev = findIMagic();    
    if(dev == NULL) {
        fprintf(stderr,"IMAGIC Device not found on any USB Bus -> please plug in\n");
        exit(1);
    }
        
    fprintf(stdout,"Found IMAGIC Device\n");
   
    if (dev->descriptor.bNumConfigurations == 0 || dev->config == NULL) {
        fprintf(stderr,"No Configuration.\n");
        exit(1);
    }

    int numconfig = 1;
    int config = 0;
    int numinterfaces = 1;
    int interface = 0;
    int num_altsetting = 3;
    int alternate = 1;
    int numEndpoint = 13;
    int readEndpointNr = 1;
    int readEndpointAdr = 0x82; 
    int writeEndpointNr = 2;
    int writeEndpointAdr = 0x02;    

    // We are lazy - we know our device and we just check if everything is as we expect it
    if(dev->descriptor.bNumConfigurations != numconfig 
        || dev->config[config].bNumInterfaces != numinterfaces
        || dev->config[config].interface[interface].num_altsetting != num_altsetting
        || dev->config[config].interface[interface].altsetting[alternate].bNumEndpoints != numEndpoint
        || dev->config[config].interface[interface].altsetting[alternate].endpoint[readEndpointNr].bEndpointAddress != readEndpointAdr
        || dev->config[config].interface[interface].altsetting[alternate].endpoint[writeEndpointNr].bEndpointAddress != writeEndpointAdr) {
        fprintf(stderr,"Wrong Configuration.\n");
        exit(1);
    }
    
    fprintf(stdout,"Found matching Configuration\n");
    struct usb_interface_descriptor* intf = &dev->config[config].interface[interface].altsetting[alternate];
    struct usb_endpoint_descriptor* readEp  = &intf->endpoint[readEndpointNr];
    struct usb_endpoint_descriptor* writeEp = &intf->endpoint[writeEndpointNr];
        
    struct usb_dev_handle* udev = usb_open(dev);
    if(udev == NULL) {
        fprintf(stderr,"Cannot open IMAGIC Device.\n");
        exit(1);
    }
                                 
    if (usb_set_configuration(udev, dev->config[config].bConfigurationValue) < 0) {
        fprintf(stderr,"Cannot set configuration.\n");
        fprintf(stderr,"check permissions on: /dev/bus/usb/ %s / %s \n", dev->bus->dirname, dev->filename);
        fprintf(stderr,"did you remember to setup a udev rule for this device?\n");
        usb_close(udev);
        exit(1);
    }
            
    if (usb_claim_interface(udev, intf->bInterfaceNumber) < 0) {
        fprintf(stderr,"Cannot claim interface\n");
        usb_close(udev);   
        exit(1);
    }
    
    if (usb_set_altinterface(udev, intf->bAlternateSetting) < 0) {
        fprintf(stderr,"Cannot switch set interface\n");
        usb_release_interface(udev, intf->bInterfaceNumber);
        usb_close(udev);   
        exit(1);    
    }
            
    usb_clear_halt(udev, writeEp->bEndpointAddress);
    usb_clear_halt(udev, readEp->bEndpointAddress);
    
    // Any starttime value is OK - just sets the inital value of the timer
    // LowByte are hundreds, the other bytes are the seconds
    uint32_t starttime = 0x00000000; 
    // Reset/Start Command with "pause" (0x01)
    char wBuf[6] = { 0x80, 0x01, starttime>>24, starttime>>16, starttime>>8, starttime };  
    int wc = usb_bulk_write(udev, writeEp->bEndpointAddress, wBuf, 6, 500 );
    if(wc != 6) {            
        fprintf(stderr,"Cannot write commands\n");
        fprintf(stderr,"firmware loaded? Set udev-rule or load firmware manually with:\n\n");
        fprintf(stderr,"\tfxload -I tacximagic.hex -D /dev/bus/usb/%s/%s\n\n", dev->bus->dirname, dev->filename);
        usb_release_interface(udev, intf->bInterfaceNumber);
        usb_close(udev);   
        exit(1);
    }
    
    printf("U=Up, D=DOwn, E=Enter, C=Cancel, ?P?=NoPowerPlug, H=Heartbeat, CAD\n");
    
    read_and_print_imagic(udev,readEp,writeEp);
            
    usb_release_interface(udev, intf->bInterfaceNumber); 
    usb_close(udev);            
                    
    return 0;
}

