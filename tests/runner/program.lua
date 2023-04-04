#!lua

a = 1

function setup()
    trace("running %d %d %d\n", 1, 2, 3)
    print("setup\n")
    val = getc()
    print("returned ",sprintf("%d",val))
end

    val = getc()
function loop2()
    print("loop\n")
end

print("main\n", sprintf("valami %d %f", 123, 3.1415))
trace("indul")
