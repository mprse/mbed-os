SPI Communication Test (One Board)
=============
 ### tests-mbed_hal-spi_com
---------
  
 **Overview**  
This is the SPI communication test which verifies various SPI configurations.
This test transfers data between SPI master and SPI slave.
It also requires to wire a SPI interface as master to another SPI interface
as slave on the same device.  
 
 **What is verified by the test?**  
 - master/slave mode,  
 - synchronous/asynchronous modes,  
 - full duplex (see note),  
 - symbol sizes: [1, 7, 8, 9, 15, 16, 17, 31, 32]  
 - clock polarity/phase,  
 - clock frequency,  
 - bit ordering during transmission,  
 - rx count equal and different from tx count,  
 - undefined rx/tx buffer (NULL values),  
 - internal/external SS handling by master.  
 - transmission of one symbol / long buffer.  

Note:  
Half-Duplex mode can not be tested. This is the limitation of the one board SPI communication test.  

 **Test scenario**  
 1. Test for synchronous api.  
     - CS is asserted high (inactive state).  
     - Configuration is validated against device capabilities.  
     - If master or slave can not handle this configuration the test case is skipped.
     - Format configuration is set for both master and slave.  
     - Frequency configuration is set on the master peripheral.  
     - Reception buffers and semaphores are reinitialized.  
     - A thread is started for the slave side.  
     - A thread is started for the master side.  
     - The master thread asserts CS to 0 (active state), performs the transfer and asserts CS back to 1.  
     - The test thread (main) waits until either the semaphore is given twice or we it reaches a timeout.  
     - Master & slave rx buffers are respectively compared to slave & master tx buffers.  
     - If a buffer do not match, then the test fails.  
     - Both SPI peripheral are freed and the next test starts.  
 2. Test for asynchronous api (if device supports SPI_ASYNCH).  
     - CS is asserted high (inactive state).  
     - Configuration is validated against device capabilities  
     - If master or slave can not handle this configuration the test case is skipped.  
     - Format configuration is set for both master and slave.  
     - Frequency configuration is set on the master peripheral.  
     - Reception buffers and semaphores are reinitialized.  
     - The spi_transfer_async function is called for the slave peripheral.  
     - The CS is asserted to 0 (active state).  
     - spi_trasnfer_async is called for the master peripheral.  
     - The CS is asserted back to 1 (inactive state).  
     - Master & slave rx buffers are respectively compared to slave & master tx buffers.  
     - If a buffer do not match, then the test fails.  
     - Both SPI peripheral are freed and the next test starts.  

Note:  
On master side data is transfered symbol by symbol (i.e. when 5 symbols is to be transfered, then transfer routine is called 5 times in loop). This is the limitation of the one board SPI communication test.  

 **Setup required to run the tests**  
In order to run these tests you need to wire the 4 pins for each peripheral as declared in the corresponding header file.  

 **Expected result:**  
 The test exits without errors.  