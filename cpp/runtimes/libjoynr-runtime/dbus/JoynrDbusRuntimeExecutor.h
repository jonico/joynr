/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

#ifndef JOYNRDBUSRUNTIMEEXECUTOR_H
#define JOYNRDBUSRUNTIMEEXECUTOR_H

#include "joynr/PrivateCopyAssign.h"
#include "runtimes/libjoynr-runtime/JoynrRuntimeExecutor.h"

namespace joynr
{

class LibJoynrRuntime;
class Settings;

class JoynrDbusRuntimeExecutor : public JoynrRuntimeExecutor
{
public:
    explicit JoynrDbusRuntimeExecutor(std::unique_ptr<Settings> settings);
    ~JoynrDbusRuntimeExecutor() override = default;

private:
    void createRuntime();
    DISALLOW_COPY_AND_ASSIGN(JoynrDbusRuntimeExecutor);
};

} // namespace joynr
#endif // JOYNRDBUSRUNTIMEEXECUTOR_H
