/*jslint es5: true, node: true, newcap: true */
/*global Buffer: true, requirejs: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

requirejs.undef("global/Smrf");
define("global/Smrf", {
    serialize : function(message) {
        var msg;
        var callback = message.signingCallback;
        if (callback && (typeof callback === "function")) {
            msg = "callback was called";
        } else {
            msg = "callback wasn't called";
        }
        return msg;
    }
});

var wscppSpy = jasmine.createSpy("wscppSyp");

requirejs.undef("wscpp");
define("wscpp", [], function() {
    return wscppSpy;
});

define([ "global/WebsocketNodeModule"
], function(WebsocketNode) {

    describe("websocket node", function() {

        var websocketNode;
        var remoteUrl = "url";
        var keychain = {
            ownerId : "ownerId"
        };
        var keychainWithCerts = {
            tlsCert : "tlsCert",
            tlsKey : "tlsKey",
            tlsCa : "tlsCa",
            ownerId : "ownerID"
        };

        var joynrMessage = {
            header : {},
            payload : "somePayload"
        };

        it("can be used without ownerId", function() {

            websocketNode = new WebsocketNode(remoteUrl);

            var serializedMessage = websocketNode.marshalJoynrMessage(joynrMessage);
            expect(serializedMessage).toBe("callback wasn't called");

        });

        it("can be used with ownerID", function() {

            websocketNode = new WebsocketNode(remoteUrl, keychain);

            var serializedMessage = websocketNode.marshalJoynrMessage(joynrMessage);
            expect(serializedMessage).toBe("callback was called");
        });

        it("calls the wscpp constructor with certs", function() {

            websocketNode = new WebsocketNode(remoteUrl, keychainWithCerts);

            expect(wscppSpy).toHaveBeenCalledWith(remoteUrl, {
                cert : keychainWithCerts.tlsCert,
                key : keychainWithCerts.tlsKey,
                ca : keychainWithCerts.tlsCa
            });

        });

        // TODO add a Test, what happens if a cert object is incomplete?

    });
});
