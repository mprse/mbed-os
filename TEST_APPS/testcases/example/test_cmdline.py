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


class Testcase(Bench):
    def __init__(self):
        Bench.__init__(self,
                       name="test_cmdline",
                       title="Smoke test for command line interface",
                       status="released",
                       purpose="Verify Command Line Interface",
                       component=["cmdline"],
                       type="smoke",
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

    def perform_test(self, spi_config):
        resp_master = self.command("master", "validate_config %s" % spi_config)
        resp_slave = self.command("slave", "validate_config %s" % spi_config)
        
        if (resp_master.success() and resp_slave.success()):
            async_cmd_slave = self.command("slave", "exec_test", asynchronous=True)
            async_cmd_master = self.command("master", "exec_test", asynchronous=True)
            
            resp_slave = self.wait_for_async_response("exec_test", async_cmd_slave)
            resp_master = self.wait_for_async_response("exec_test", async_cmd_master)
        return True

    def setup(self):
        pass

    def case(self):
        resp_slave = self.command("slave", "echo off")
        resp_slave = self.command("master", "echo off")
        
        spi_config = "symbol_size %d bit_ordering %d freq_hz %d master_tx_cnt %d master_rx_cnt %d slave_tx_cnt %d slave_rx_cnt %d master_tx_defined %s master_rx_defined %s slave_tx_defined %s slave_rx_defined %s auto_ss %s duplex %d sync %s" % (8, 0, 100000, 5, 5, 5, 5, 'true', 'true', 'true', 'true', 'true', 0, 'true')
        self.assertTrue(self.perform_test(spi_config), message=None)

    def teardown(self):
        pass
