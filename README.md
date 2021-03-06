## Information

dither_png is a command-line program that takes an input .png file and creates a new .png file that is a 1-bit (black and white) version of the original using dithering.

<img src="img/santorini.png" alt="santorini original pic">
<img src="img/santorini_dithered.png" alt="santorini dithered pic">

## Requirements

A system with gcc and libpng


## Installation

Install [libpng](http://www.libpng.org/pub/png/libpng.html), then run 

```
$ make
```

This will build dither_png at ```bin/dither_png```

## Usage

Running
```
$ dither_png cats.png
```
will create a file named ```out.png```. By default, the output is saved in ```out.png```.

By providing a second argument, you can save it using a specific filename. Running
```
$ dither_png cats.png dogs.png
```
will save the output to ```dogs.png```

By default, dither_png uses ordered dithering. To use, alternatively, Floyd-Steinberg dithering, use ```-f```
```
$ dither_png -f cats.png
```

