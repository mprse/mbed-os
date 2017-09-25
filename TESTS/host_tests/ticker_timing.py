"""
mbed SDK
Copyright (c) 2011-2013 ARM Limited

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

from mbed_host_tests import BaseHostTest
import time

DELAY_10S = 10

class TickerTimingTests(BaseHostTest):
    """
    This is the host part of the test to verify if mbed board ticker freqency is valid.
    Test scenario:
    1) Device after is booted up sends its defined ticker clock frequency [Hz] to the host.
    2) Host sends "start" command to the device and the device stores current ticker tick count.
    3) Host sleeps for 10 s and then sends "stop_" command to the device and the device stores current ticker tick count.
    4) Device sends to the host the difference between stop and start tick count
       (how many ticks have elapsed in 10 s time period).
    5) Host calculates counted frequency:
       calculated frequency = counted ticks / 10 [s]
    6) Host verifies if defined board frequency consisten with couned frequency and sends result to the device.
    
    Note:
    It has been assumed that the deviation beetween defined frequency and counted frequency can be +/- 10%.
    
    It has been assumed that the delay for transmission is constant (start and stop messages have the same length)
    and irrelevant.
    
         start               stop
    PC    -*--------10s-------*--------  
            \                  \
             \                  \      transmission
              \                  \
    DEV   -----*-------10s--------*----  
             start               stop

    """
    def _ticker_clk_freq(self, key, value, timestamp):
        self.ticker_clk_freq = int(value)
        self.freq_delta_10_prc = self.ticker_clk_freq / 10
        self.send_kv("start", 0)
        time.sleep(DELAY_10S)
        self.send_kv("stop_", 0)
        
    def _total_tick_count(self, key, value, timestamp):
        self.total_tick_count = int(value)
        self.counted_freq = self.total_tick_count / DELAY_10S
        
        if(self.ticker_clk_freq > (self.counted_freq + self.freq_delta_10_prc) or 
           self.ticker_clk_freq < (self.counted_freq - self.freq_delta_10_prc)):
            self.send_kv("failed", 0)
        else:
            self.send_kv("passed", 0)
        
    def setup(self):
        self.register_callback('ticker_clk_freq', self._ticker_clk_freq)
        self.register_callback('total_tick_count', self._total_tick_count)

    def result(self):
        return self.__result

    def teardown(self):
        pass
