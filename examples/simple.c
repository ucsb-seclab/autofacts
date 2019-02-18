#include<stdio.h>

int foo(int b) {
    int c;
    c  = b * 4;
    while(c) {
        c--;
        b += 3;
    }
    
    c += b;
    printf("In foo\n");
    return c;
}

int baz(long b) {
    int c;
    c  = b * 4;
    while(c) {
        c--;
        b += foo(b);
        
    }
    
    c += b;
    printf("In baz\n");
    return c;
}

int bar(long b) {
    int c;
    c  = b * 4;
    c += foo(c);
    
    while(c) {
        c--;
        b += baz(b);
        
    }     
    
    printf("In bar\n");
    c += b;
    
    return c;
}

int mklo(int b) {
    int c;
    c  = b * 4;
    
    c += foo(c);
    c += bar(c);
    
    while(c) {
        c--;
        b += baz(b);
        
    }     
    
    printf("In mklo\n");
    c += b;
    
    return c;
}



