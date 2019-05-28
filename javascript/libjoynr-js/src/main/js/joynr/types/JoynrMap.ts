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
/* istanbul ignore file */

import JoynrObject from "./JoynrObject";

class JoynrMap<T> extends JoynrObject {
    protected constructor(settings: Record<string, T> | undefined) {
        super();
        if (settings !== undefined) {
            for (const settingKey in settings) {
                if (Object.prototype.hasOwnProperty.call(settings, settingKey)) {
                    // @ts-ignore
                    this[settingKey] = settings[settingKey];
                }
            }
        }
    }

    public put(key: string, value: T): void {
        // @ts-ignore
        this[key] = value;
    }

    public get(key: string): T {
        // @ts-ignore
        return this[key];
    }

    public remove(key: string): void {
        // @ts-ignore
        delete this[key];
    }
}
export = JoynrMap;
