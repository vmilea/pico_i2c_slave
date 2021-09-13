# I2C slave library for the Raspberry Pi Pico

The Raspberry Pi Pico C/C++ SDK has all you need to write an I2C master, but is curiously lacking when it comes to I2C in slave mode. This library fills that gap to easily turn the Pico into an I2C slave.

## Example

An example program is included where the slave acts as a 256 byte external memory.

To keep it simple, both master and slave run on the same board. Just wire up the two I2C instances: GP4 to GP6 (SDA), and GP5 to GP7 (SCL).

### Building

Follow the instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to setup your build environment. Then:

- clone repo
- `mkdir build`, `cd build`, `cmake ../`, `make`
- copy `i2c_slave_example.uf2` to Raspberry Pico
- open a serial connection and check output

## Links

- [Pico as I2C slave](https://www.raspberrypi.org/forums/viewtopic.php?t=304074) - basic setup using raw registers
- [DroneBot Workshop](https://dronebotworkshop.com/i2c-part-2-build-i2c-sensor/) - building an I2C sensor

## Authors

Valentin Milea <valentin.milea@gmail.com>
