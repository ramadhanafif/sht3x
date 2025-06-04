# SHT3x Sensor Library for ESP

This project provides a library for interfacing with SHT3x series temperature and humidity sensors using an ESP microcontroller.

This project was forked from [espressif/esp-iot-solution for sht3x](https://github.com/espressif/esp-iot-solution/tree/350117fb02941f9d0f3a2ccde53f8da108d2ee8a/components/sensors/humiture/sht3x).

## Example Usage

An example application is provided in the `example` directory, demonstrating how to initialize the I2C bus, create a sensor instance, and read temperature and humidity data in a loop.

See [`example/main/sht3x-example.c`](example/main/sht3x-example.c) for the example code.

## Building and Running the Example

1. Navigate to the `example` directory.
2. Configure the project using `idf.py menuconfig`.
3. Build the project using `idf.py build`.
4. Flash the project to your ESP board using `idf.py flash`.
5. Monitor the serial output using `idf.py monitor`.

## Contributing

Contributions are welcome. Please follow the standard GitHub flow for contributions.

## License

This project is licensed under the Apache 2.0 License, following the espressif ibrary noted above.