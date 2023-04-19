#!c

#define A 1+2*3+sizeof int + sizeof(int32_t)
#define IN int

#ifdef A
enum { B = 1, C, D };
#endif

IN a;
char *b = "valami" "masik";
int c[4][2] = { {1,2}, {2,2-3}, {2+1,4}, {4,1} };
float f = .5f;

void d()
{
    e();
}

void e()
{
    debug;
    printf("Hello World\n");
}

void setup()
{
    trace("running %d %d %d %s %d %s %f\n", 1, 2, 3, b, c[1][1], "valami", f);
    a = 1;
    a ? d() : e();
    if(a) d();
}

void loop(void)
{
    int g;
    if(a == B && !g || a) debug;
}
