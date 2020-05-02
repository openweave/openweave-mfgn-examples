#
#   Copyright (c) 2019 Google LLC.
#   All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

# 
#   @file
#         Makefile for building the OpenWeave Sample Applications.
#

PROJECT_ROOT := $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

ifeq ($(PLATFORM),nrf5)
    include $(PROJECT_ROOT)/nrf5-app.mk
else ifeq ($(APP),silabs)
    include $(PROJECT_ROOT)/silabs.mk
endif
