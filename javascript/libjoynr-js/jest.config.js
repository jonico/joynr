module.exports = {
    transform: {
        "\\.ts$": "ts-jest"
    },
    testEnvironment: "node",
    testRegex: "src/test/.*Test\\.ts$",
    testPathIgnorePatterns: ["/node_modules/", "node_integration"],
    moduleFileExtensions: ["ts", "js", "json", "node"],
    reporters: [
        "default",
        [
            "jest-junit",
            {
                output: "target/test-results/ts-junit.xml"
            }
        ]
    ],
    coverageThreshold: {
        global: {
            branches: 65,
            functions: 80,
            lines: 80,
            statements: 80
        }
    },
    coverageDirectory: ".build/coverage/ts",
    collectCoverageFrom: ["src/main/js/**/*.ts", "!src/**/*.d.ts"],
    coveragePathIgnorePatterns: ["/src/main/js/generated", "/node_modules"],
    coverageReporters: ["cobertura", "lcov"],
    modulePathIgnorePatterns: ["target"],
    testRunner: "jest-circus/runner"
};
