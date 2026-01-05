
1. Allman braces

```c++```
int foo()
{
    if (a)
    {
    }
    for (int : x)
    {
    }
}

struct x
{
};

```c++```

2. Snake casing

```
indentifiers_are_like_this
```

3. Suffix _ in data member names

```c++```
struct x
{
    int a_;
    int b_;
};

class y
{
    int a_;
    int b_;
};

```c++```

4. Don't add unnecessary spaces

5. No one-liners

```c++```
// correct, preferred
if (expr)
    do_stuff();

for (int i = 0; i < n; ++i)
    do_stuff();

// also acceptable
if (expr)
{
    do_stuff();
}
        
for (int i = 0; i < n; ++i)
{
    do_stuff();
}
    
// incorrect 
if (expr) do_stuff();
for (int i = 0; i < n; ++i) do_stuff();
```c++```
