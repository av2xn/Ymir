// ymir.c
// Generic USB bulk streaming tool, fully configurable

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sysexits.h>
#include <libusb-1.0/libusb.h>

static int verbose = 0;
#define LOG(...) do { if(verbose) fprintf(stderr, __VA_ARGS__); } while(0)

static void usage(const char *p){
    fprintf(stderr,
            "Usage: %s --vid 0xVVVV --pid 0xPPPP --ep-out 0xNN --ep-in 0xMM --iface N [options]\n"
            "Options:\n"
            "  --verbose                Enable verbose logging\n"
            "  --timeout MS             Set transfer timeout in milliseconds (default 5000)\n"
            "  --chunk-size N           Set transfer chunk size in bytes (default 4096)\n"
            "Examples:\n"
            "  echo -n \"DATA_TO_SEND\" | sudo %s --vid 0xVVVV --pid 0xPPPP --ep-out 0xNN --ep-in 0xMM --iface N --verbose --timeout 5000 --chunk-size 4096 | hexdump -C\n",
            p, p);
}

static int parse_hex(const char *s, int *out){
    if(!s) return -1;
    char *end = NULL;
    long v = strtol(s, &end, 0);
    if(end == s || *end != '\0') return -1;
    *out = (int)v;
    return 0;
}

int main(int argc, char **argv){
    if(argc < 9){
        usage(argv[0]);
        return EX_USAGE;
    }

    int vid=-1, pid=-1, ep_out=-1, ep_in=-1, iface=-1;
    int timeout = 5000;
    int chunk_size = 4096;

    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--vid")==0 && i+1<argc) { if(parse_hex(argv[++i], &vid)!=0){ usage(argv[0]); return EX_USAGE; } }
        else if(strcmp(argv[i],"--pid")==0 && i+1<argc) { if(parse_hex(argv[++i], &pid)!=0){ usage(argv[0]); return EX_USAGE; } }
        else if(strcmp(argv[i],"--ep-out")==0 && i+1<argc) { if(parse_hex(argv[++i], &ep_out)!=0){ usage(argv[0]); return EX_USAGE; } }
        else if(strcmp(argv[i],"--ep-in")==0 && i+1<argc) { if(parse_hex(argv[++i], &ep_in)!=0){ usage(argv[0]); return EX_USAGE; } }
        else if(strcmp(argv[i],"--iface")==0 && i+1<argc) { if(parse_hex(argv[++i], &iface)!=0){ usage(argv[0]); return EX_USAGE; } }
        else if(strcmp(argv[i],"--timeout")==0 && i+1<argc) { timeout = atoi(argv[++i]); }
        else if(strcmp(argv[i],"--chunk-size")==0 && i+1<argc) { chunk_size = atoi(argv[++i]); }
        else if(strcmp(argv[i],"--verbose")==0) { verbose = 1; }
        else if(strcmp(argv[i],"--help")==0) { usage(argv[0]); return 0; }
        else { usage(argv[0]); return EX_USAGE; }
    }

    if(vid < 0 || pid < 0 || ep_out < 0 || ep_in < 0 || iface < 0){
        usage(argv[0]);
        return EX_USAGE;
    }

    if (isatty(STDOUT_FILENO)) {
        fprintf(stderr, "Warning: Binary output is being written to terminal!\n");
    }

    libusb_context *ctx = NULL;
    if(libusb_init(&ctx) != 0){
        fprintf(stderr,"libusb_init failed\n");
        return EX_UNAVAILABLE;
    }

    libusb_device_handle *h = libusb_open_device_with_vid_pid(ctx, (uint16_t)vid, (uint16_t)pid);
    if(!h){
        fprintf(stderr,"Device 0x%04X:0x%04X not found or permission denied\n", vid, pid);
        libusb_exit(ctx);
        return EX_UNAVAILABLE;
    }

    int kernel_attached = 0;
    if(libusb_kernel_driver_active(h, iface) == 1){
        if(libusb_detach_kernel_driver(h, iface) != 0){
            fprintf(stderr, "Failed to detach kernel driver on iface %d\n", iface);
            libusb_close(h); libusb_exit(ctx); return EX_NOPERM;
        }
        kernel_attached = 1;
    }

    if(libusb_claim_interface(h, iface) != 0){
        fprintf(stderr, "Claim interface %d failed\n", iface);
        libusb_close(h); libusb_exit(ctx); return EX_UNAVAILABLE;
    }

    unsigned char *buf = malloc(chunk_size);
    if(!buf){ fprintf(stderr,"Failed to allocate buffer\n"); libusb_release_interface(h, iface); libusb_close(h); libusb_exit(ctx); return EX_OSERR; }

    size_t r;
    while((r = fread(buf, 1, chunk_size, stdin)) > 0){
        int transferred = 0;
        int ret = libusb_bulk_transfer(h, (unsigned char)ep_out, buf, (int)r, &transferred, timeout);
        if(ret != 0){
            fprintf(stderr, "Write error: %s\n", libusb_error_name(ret));
            break;
        }
        LOG("Sent %d bytes to ep-out 0x%02X\n", transferred, ep_out);

        int in_trans = 0;
        ret = libusb_bulk_transfer(h, (unsigned char)ep_in, buf, chunk_size, &in_trans, timeout);
        if(ret == LIBUSB_ERROR_TIMEOUT){
            LOG("Read timeout (no response)\n");
            continue;
        } else if(ret != 0){
            fprintf(stderr, "Read error: %s\n", libusb_error_name(ret));
            break;
        }

        if(in_trans > 0){
            fwrite(buf, 1, in_trans, stdout);
            fflush(stdout);
            LOG("\nReceived %d bytes from ep-in 0x%02X\n", in_trans, ep_in);
        }
    }

    free(buf);
    libusb_release_interface(h, iface);
    if(kernel_attached && libusb_kernel_driver_active(h, iface) == 0){
        libusb_attach_kernel_driver(h, iface);
    }

    libusb_close(h);
    libusb_exit(ctx);
    return 0;
}
