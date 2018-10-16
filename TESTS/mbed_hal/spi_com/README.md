SPI Communication Test
=============

### tests-mbed_hal-spi_com

**Description**  
This is the SPI communication test which verifies various SPI configurations.
This test transfers data between SPI master and SPI slave.
Two boards are required with SPI peripheral available and slave/master mode support.    

To run the test perform the following steps:  
 1. Adjust configuration to your needs in .TESTS/mbed_hal/spi_com/mbed_app.json 
 2. Connect SPI interfaces as configured in .TESTS/mbed_hal/spi_com/mbed_app.json  
 3. Execute the following command to build spi slave test application:  
 mbed test -t TOOLCHAIN -m TARGET -n tests-mbed_hal-spi_com --compile -DIS_MASTER=0 --app-config ./TESTS/mbed_hal/spi_com/mbed_app.json  
 4. Flash the slave board with the build slave test application and reset the board.
 5. Execute the following command to build spi master test application:  
  mbed test -t TOOLCHAIN -m TARGET -n tests-mbed_hal-spi_com --compile -DIS_MASTER=1 --app-config ./TESTS/mbed_hal/spi_com/mbed_app.json  
 6. Run the test using the following command:  
 mbed test -t TOOLCHAIN -m TARGET -n tests-mbed_hal-spi_com --run --use-tids MASTER_TARGET_ID  

**Which SPI features are verified by the test?**  
 - master/slave mode,  
 - synchronous/asynchronous modes,  
 - full duplex/half duplex modes,  
 - symbol sizes: [1, 32],  
 - clock polarity/phase,  
 - bit ordering during transmission,  
 - clock frequency,  
 - rx count == tx count, tx count != tx count,  
 - undefined rx/tx buffer,  
 - internal/external ss handling by master.  

**Test scenario**  
Test assumes that default configuration:  
 - 8 bit symbol,  
 - sync mode,  
 - full duplex,  
 - clock idle low,  
 - sample on the first clock edge,  
 - MSB transmitted first,  
 - 200 KHz clock clock,  
 - internal SS handling,  
  is supported by all SPI devices.  

Test steps:  
 - Configuration is validated against the capabilities on master side.  
 - If master can not handle this configuration - test case is skipped.  
 - Master sends the configuration to slave using default configuration.  
 - Configuration is validated against the capabilities on slave side.  
 - If slave can not handle this configuration it sends CONFIG_STATUS_NOT_SUPPORTED status to master and test case is skipped. Otherwise slave sends CONFIG_STATUS_OK to the master.  
 - When both sides can handle the specified configuration, then slave and master loads the configuration and perform communication test.    

**Expected result:**  
 The test exits without failures.

