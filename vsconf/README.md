# Yet another configuration file!

## C API

To configure your application, just call `configure` function with the configuration file path as argument:

```c

#include <vsconf.h>
#include <glib.h>

int main(int argc, char *argv[]) {
    configure("path/to/config/file");
    // ...
    return 0;
}

```

The variables are accessible through the `get` function:

```c

#include <vsconf.h>

int main(int argc, char *argv[]) {
    configure("path/to/config/file");
    int* const num_ports = (int*)get("PORTS");
    if(num_ports != NULL) {
        printf("num_ports = %d\n", *num_ports);
    }
    // ...
    return 0;
}

```


The configuration file is a simple text file with the following format:

```
VARIABLE = value
```

For example:

```
PORTS = 24
FST = false
```

