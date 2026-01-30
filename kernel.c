#include <stdint.h>

__attribute__((section(".multiboot")))
const uint32_t multiboot_header[] = {0x1BADB002, 0, -(0x1BADB002)};

volatile uint16_t* vga = (uint16_t*)0xB8000;
int cursor = 0;

#define MAX_INPUT 64
#define MAX_FILES 16
#define MAX_CONTENT 128

char input[MAX_INPUT];
int input_pos = 0;
volatile uint32_t ticks = 0;

struct file {
    char name[16];
    char content[MAX_CONTENT];
    int used;
};

struct file fs[MAX_FILES];

static inline uint8_t inb(uint16_t p){
    uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r;
}
static inline void outb(uint16_t p,uint8_t v){
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}

void putc(char c){ vga[cursor++] = (0x1F<<8)|c; }
void print(const char*s){ while(*s) putc(*s++); }
void newline(){ cursor=(cursor/80+1)*80; }
void clear_screen(){
    for(int i=0;i<80*25;i++) vga[i]=(0x1F<<8)|' ';
    cursor=0;
}

char map[128]={
0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,
9,'q','w','e','r','t','y','u','i','o','p','[',']','\n',
0,'a','s','d','f','g','h','j','k','l',';','\'','`',
0,'\\','z','x','c','v','b','n','m',',','.','/',
0,'*',0,' '
};

int streq(const char*a,const char*b){
    int i=0; while(a[i]&&b[i]){if(a[i]!=b[i])return 0;i++;} 
    return a[i]==0&&b[i]==0;
}
int starts(const char*s,const char*p){
    int i=0; while(p[i]){if(s[i]!=p[i])return 0;i++;} return 1;
}
void strcopy(char*d,const char*s){
    int i=0; while(s[i]){d[i]=s[i];i++;} d[i]=0;
}

void print_num(int n){
    char buf[16]; int i=0;
    if(n==0) buf[i++]='0';
    while(n){buf[i++]='0'+n%10;n/=10;}
    for(int j=i-1;j>=0;j--) putc(buf[j]);
}

uint8_t cmos_read(uint8_t r){ outb(0x70,r); return inb(0x71); }
uint8_t bcd(uint8_t v){ return (v&0x0F)+((v/16)*10); }

void cmd_date(){
    int d=bcd(cmos_read(7));
    int m=bcd(cmos_read(8));
    int y=bcd(cmos_read(9));
    int h=bcd(cmos_read(4));
    int min=bcd(cmos_read(2));
    print("Date "); print_num(d); putc('/'); print_num(m); putc('/'); print_num(2000+y);
    newline();
    print("Time "); print_num(h); putc(':'); print_num(min);
}

void reboot(){ outb(0x64,0xFE); }
void shutdown(){ outb(0x604,0x2000); }

void fs_list(){
    for(int i=0;i<MAX_FILES;i++){
        if(fs[i].used){
            print(fs[i].name);
            newline();
        }
    }
}

void fs_create(const char*name){
    for(int i=0;i<MAX_FILES;i++){
        if(!fs[i].used){
            fs[i].used=1;
            strcopy(fs[i].name,name);
            fs[i].content[0]=0;
            print("File created");
            return;
        }
    }
    print("Filesystem full");
}

void fs_cat(const char*name){
    for(int i=0;i<MAX_FILES;i++){
        if(fs[i].used && streq(fs[i].name,name)){
            print(fs[i].content);
            return;
        }
    }
    print("No such file");
}

void fs_write(const char*name,const char*txt){
    for(int i=0;i<MAX_FILES;i++){
        if(fs[i].used && streq(fs[i].name,name)){
            strcopy(fs[i].content,txt);
            print("Written");
            return;
        }
    }
    print("No such file");
}

/* Sleep */
void sleep_ticks(uint32_t t){
    uint32_t s=ticks;
    while(ticks-s<t);
}

void handle_command(){
    newline();

    if(streq(input,"help"))
        print("FlopOS - help clear date list create cat echo sleep reboot shutdown");
    else if(streq(input,"clear"))
        clear_screen();
    else if(streq(input,"date"))
        cmd_date();
    else if(streq(input,"list"))
        fs_list();
    else if(starts(input,"create "))
        fs_create(input+7);
    else if(starts(input,"cat "))
        fs_cat(input+4);
    else if(starts(input,"echo ") && starts(input+5,"") ){
        char*gt=0;
        for(int i=0;input[i];i++)
            if(input[i]=='>' && input[i-1]==' ') gt=&input[i];

        if(gt){
            *gt=0;
            fs_write(gt+2,input+5);
        }else{
            print(input+5);
        }
    }
    else if(starts(input,"sleep ")){
        int n=0;
        for(int i=6;input[i];i++) n=n*10+(input[i]-'0');
        sleep_ticks(n*100000);
    }
    else if(streq(input,"reboot"))
        reboot();
    else if(streq(input,"shutdown"))
        shutdown();
    else
        print("Unknown command");

    newline();
    input_pos=0;
    input[0]=0;
    print("> ");
}

void kernel_main(){
    clear_screen();
    print("FlopOS shell \n> ");

    while(1){
        ticks++;

        if(inb(0x64)&1){
            uint8_t sc=inb(0x60);
            if(sc&0x80) continue;

            char c=map[sc];

            if(c=='\n'){
                input[input_pos]=0;
                handle_command();
            }
            else if(c==8){
                if(input_pos){
                    input_pos--;
                    cursor--;
                    vga[cursor]=(0x1F<<8)|' ';
                }
            }
            else if(c && input_pos<MAX_INPUT-1){
                input[input_pos++]=c;
                putc(c);
            }
        }
    }
}

