/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import End2EndAbstractTest from "../End2EndAbstractTest";

describe("Compatibility tests for providers created by new generator", () => {
    it("Provider with version in package name and original proxy", async () => {
        const abstractTest = new End2EndAbstractTest(
            "ProviderWithVersionedPackageNameTest",
            "TestMultipleVersionsInterfaceProcess",
            { versioning: "packageVersion2" }
        );

        return abstractTest.beforeEach().then(abstractTest.afterEach);
    });

    it("Provider with version in name and original proxy", async () => {
        const abstractTest = new End2EndAbstractTest(
            "ProviderWithVersionedNameTest",
            "TestMultipleVersionsInterfaceProcess",
            { versioning: "nameVersion2" }
        );

        return abstractTest.beforeEach().then(abstractTest.afterEach);
    });
});
