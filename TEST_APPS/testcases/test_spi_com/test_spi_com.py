"""
Copyright 2018 ARM Limited
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
from icetea_lib.bench import Bench

SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE = 0
SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE = 1
SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE = 2
SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE = 3

SPI_BIT_ORDERING_MSB_FIRST = 0
SPI_BIT_ORDERING_LSB_FIRST = 1

SPI_SYNC_MASTER_SYNC_SLAVE = 3
SPI_ASYNC_MASTER_SYNC_SLAVE = 2
SPI_SYNC_MASTER_ASYNC_SLAVE = 1
SPI_ASYNC_MASTER_ASYNC_SLAVE = 0

FULL_DUPLEX = 0
HALF_DUPLEX_MOSI = 1
HALF_DUPLEX_MISO = 2

FREQ_MIN = 0
FREQ_MAX = -1

CONFIG_STRING  = "symbol_size %d mode %d bit_ordering %d freq_hz %d master_tx_cnt %d master_rx_cnt %d slave_tx_cnt %d slave_rx_cnt %d master_tx_defined %s master_rx_defined %s slave_tx_defined %s slave_rx_defined %s auto_ss %s duplex %d sync %d"

test_cases = [  # default config: 8 bit symbol\sync mode\full duplex\clock idle low\sample on the first clock edge\MSB first\100 KHz clock\manual SS handling
#                 {'info': 'default config'                               , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # symbol size testing
#                 {'info': 'symbol size: 1 bit'                           , 'symbol_size': 1 , 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'symbol size: 7 bits'                          , 'symbol_size': 7 , 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'symbol size: 9 bits'                          , 'symbol_size': 9 , 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'symbol size: 15 bits'                         , 'symbol_size': 15, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
                 {'info': 'symbol size: 16 bits'                         , 'symbol_size': 16, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'symbol size: 17 bits'                         , 'symbol_size': 17, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'symbol size: 31 bits'                         , 'symbol_size': 31, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'symbol size: 32 bits'                         , 'symbol_size': 32, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # mode testing
#                 {'info': 'mode: clk idle low/sample on second clk egde' , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_SECOND_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'mode: clk idle high/sample on first clk egde' , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_HIGH_SAMPLE_FIRST_EDGE , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'mode: clk idle high/sample on second clk egde', 'symbol_size': 8, 'mode': SPI_MODE_IDLE_HIGH_SAMPLE_SECOND_EDGE, 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # bit ordering testing
#                 {'info': 'bit ordering: LBS first'                      , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_LSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # freq testing
#                 {'info': 'freq: min required'                           , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 200000    , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'freq: max required'                           , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 2000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'freq: min defined by capabilities'            , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': FREQ_MIN  , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'freq: max defined by capabilities'            , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': FREQ_MAX  , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # master: TX > RX
#                  {'info': 'buffers: master TX > master RX'               , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 3, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # master: TX < RX
#                 {'info': 'buffers: master TX < master RX'               , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE   , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 3, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # slave: TX > RX
#                 {'info': 'buffers: slave TX > slave RX'                 , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 3, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # slave: TX < RX
#                 {'info': 'buffers: slave TX < slave RX'                 , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 3, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # master tx buffer undefined
#                 {'info': 'buffers: master TX undefined'                 , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'false', 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # master rx buffer undefined
#                 {'info': 'buffers: master RX undefined'                 , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'false', 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # slave tx buffer undefined
#                 {'info': 'buffers: slave TX undefined'                  , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'false', 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # slave rx buffer undefined
#                 {'info': 'buffers: slave RX undefined'                  , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'false', 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # auto ss hadling by master
#                 {'info': 'slave select: auto ss handling by master'     , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'true' , 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # half duplex mode
#                 {'info': 'half duplex on MOSI'                          , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': HALF_DUPLEX_MOSI,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 {'info': 'half duplex on MISO'                          , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': HALF_DUPLEX_MISO,'sync': SPI_SYNC_MASTER_SYNC_SLAVE  },
#                 # async mode
#                 {'info': 'master async mode'                            , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_ASYNC_MASTER_SYNC_SLAVE },
#                 {'info': 'slave async mode'                             , 'symbol_size': 8, 'mode': SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE  , 'bit_ordering': SPI_BIT_ORDERING_MSB_FIRST, 'freq_hz': 1000000   , 'master_tx_cnt': 5, 'master_rx_cnt': 5, 'slave_tx_cnt': 5, 'slave_rx_cnt': 5, 'master_tx_defined': 'true' , 'master_rx_defined': 'true' , 'slave_tx_defined': 'true' , 'slave_rx_defined': 'true' , 'auto_ss': 'false', 'duplex': FULL_DUPLEX     ,'sync': SPI_SYNC_MASTER_ASYNC_SLAVE }
                ]

class Testcase(Bench):
    def __init__(self):
        Bench.__init__(self,
                       name="test_spi_com",
                       title="SPI Communication Test",
                       status="released",
                       purpose="Verify new SPI HAL API",
                       component=["SPI"],
                       type="compatibility",
                       requirements={
                           "duts": {
                               '*': {
                                   "count": 2,
                                   "type": "hardware"
                               },
                               "1": {"nick": "master",
                                     "application": {
                                       "name": "TEST_APPS-device-spi_master"
                                   }},
                               "2": {"nick": "slave",
                                     "application": {
                                       "name": "TEST_APPS-device-spi_slave"
                                   }}
                           }
                       }
                       )
    
    result = "PASSED"
    
    def perform_test(self, tc_config):
        
        config_str = CONFIG_STRING % (tc_config['symbol_size'], 
                                      tc_config['mode'], 
                                      tc_config['bit_ordering'],
                                      tc_config['freq_hz'], 
                                      tc_config['master_tx_cnt'], 
                                      tc_config['master_rx_cnt'], 
                                      tc_config['slave_tx_cnt'], 
                                      tc_config['slave_rx_cnt'], 
                                      tc_config['master_tx_defined'], 
                                      tc_config['master_rx_defined'], 
                                      tc_config['slave_tx_defined'], 
                                      tc_config['slave_rx_defined'], 
                                      tc_config['auto_ss'], 
                                      tc_config['duplex'], 
                                      tc_config['sync'])
        
        resp_master = self.command("master", "validate_config %s" % config_str)
        resp_slave = self.command("slave", "validate_config %s" % config_str)
        
        if (not resp_master.verify_trace("SKIP", break_in_fail=False) and not resp_master.verify_trace("SKIP", break_in_fail=False)):
            resp_master = self.command("master", "init_test")
            resp_slave = self.command("slave", "init_test")
            
            async_cmd_slave = self.command("slave", "exec_test", asynchronous=True)
            resp_master = self.command("master", "exec_test", report_cmd_fail=False)
            
            resp_slave = self.wait_for_async_response("exec_test", async_cmd_slave)
            
            master_error = resp_master.verify_trace("ERROR", break_in_fail=False)
            slave_error = resp_slave.verify_trace("ERROR", break_in_fail=False)
            
            resp_master = self.command("master", "finish_test")
            resp_slave = self.command("slave", "finish_test")
            
            if(master_error or slave_error):
                self.result = "FAILED"
            else:
                self.result = "PASSED"
            #self.assertFalse(master_error)
            #self.assertFalse(slave_error)
        else:
            self.result = "SKIPPED"

    def setup(self):
        print("\r\n")
        pass

    def case(self):
        print('%-50s%-10s' % ("SUB TEST CASE", "STATUS"))
        print("--------------------------------------------------------")
        for tc_config in test_cases:
            self.perform_test(tc_config)
            print('%-50s%-10s' % (tc_config['info'], self.result))

    def teardown(self):
        print("\r\n")
        pass