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
import calendar
import datetime

class SPI_sync(BaseHostTest):
    """
    This is the host part of the test to verify if:
    - _rtc_mktime function converts a calendar time into time since UNIX epoch as a time_t,
    - _rtc_localtime function converts a given time in seconds since epoch into calendar time.
    
    The same algoritm to generate next calendar time to be tested is used by both parts of the test.
    We will check if correct time since UNIX epoch is calculated for the first and the last day
    of each month and across valid years.
    
    Mbed part of the test sends calculated time since UNIX epoch.
    This part validates given value and responds to indicate pass or fail.
    Additionally it sends also encoded day of week and day of year which
    will be needed to verify _rtc_localtime.
    
    Support for both types of RTC devices is provided:
    - RTCs which handles all leap years in the mentioned year range correctly. Leap year is determined by checking if
      the year counter value is divisible by 400, 100, and 4. No problem here.
    - RTCs which handles leap years correctly up to 2100. The RTC does a simple bit comparison to see if the two
      lowest order bits of the year counter are zero. In this case 2100 year will be considered
      incorrectly as a leap year, so the last valid point in time will be 28.02.2100 23:59:59 and next day will be
      29.02.2100 (invalid). So after 28.02.2100 the day counter will be off by a day.
      
    """

    def _verify_timestamp(self, key, value, timestamp):


    def setup(self):
        self.register_callback('timestamp', self._verify_timestamp)
