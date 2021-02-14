// Demo JavaScript in SciteQt
// Run this script by pressing Ctrl+J

function f(x)
{
    var y = x * x;
    return y + 3;
}

println("Hello world for JavaScript in SciteQt!");

for( i=0; i<10; i++ )
{
    println(i," ",f(i));
}

messageBox("Script done.")
