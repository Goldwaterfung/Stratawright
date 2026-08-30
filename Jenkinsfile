pipeline {
    agent any

    options {
        disableConcurrentBuilds()
        buildDiscarder(logRotator(numToKeepStr: '15'))
    }

    parameters {
        choice(
            name: 'BUILD_TYPE',
            choices: ['Debug', 'Release', 'RelWithDebInfo'],
            description: 'CMake build configuration'
        )
        booleanParam(
            name: 'RUN_TESTS',
            defaultValue: true,
            description: 'Run automated CTest unit tests'
        )
        booleanParam(
            name: 'PACKAGE_RELEASE',
            defaultValue: false,
            description: 'Generate CPack installer packages (.pkg on macOS, .exe on Windows)'
        )
        booleanParam(
            name: 'CLEAN_BUILD',
            defaultValue: false,
            description: 'Clean the build directory before building'
        )
    }

    environment {
        PATH = "/opt/homebrew/bin:/usr/local/bin:${env.PATH}"
        CMAKE_BUILD_DIR = "build/${params.BUILD_TYPE.toLowerCase()}"
        VCPKG_DEFAULT_BINARY_CACHE = "${WORKSPACE}/.vcpkg_cache"
    }

    stages {
        stage('Initialize Workspace') {
            steps {
                script {
                    echo "=========================================="
                    echo "Starting Jenkins Build for Stratawright"
                    echo "Build Configuration : ${params.BUILD_TYPE}"
                    echo "Build Directory     : ${env.CMAKE_BUILD_DIR}"
                    echo "Run Tests           : ${params.RUN_TESTS}"
                    echo "Package Release     : ${params.PACKAGE_RELEASE}"
                    echo "Workspace           : ${WORKSPACE}"
                    echo "=========================================="
                }
                sh '''
                    mkdir -p "${VCPKG_DEFAULT_BINARY_CACHE}"
                    if [ "${CLEAN_BUILD}" = "true" ]; then
                        echo "Cleaning build directory: ${CMAKE_BUILD_DIR}..."
                        rm -rf "${CMAKE_BUILD_DIR}"
                    fi
                    mkdir -p "${CMAKE_BUILD_DIR}"
                '''
            }
        }

        stage('Setup Dependencies & SDKs') {
            steps {
                echo "Fetching third-party SDKs and bootstrapping vcpkg..."
                sh '''
                    bash scripts/setup_third_party.sh
                    bash scripts/setup_vcpkg.sh
                '''
            }
        }

        stage('Configure CMake') {
            steps {
                echo "Configuring CMake project..."
                sh '''
                    cmake -B "${CMAKE_BUILD_DIR}" -S . \
                        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
                        -DBUILD_TESTS=ON \
                        -DENABLE_SIMD=ON \
                        -DCMAKE_TOOLCHAIN_FILE="${WORKSPACE}/vcpkg/scripts/buildsystems/vcpkg.cmake"
                '''
            }
        }

        stage('Compile') {
            steps {
                echo "Building project in parallel across host CPU cores..."
                sh '''
                    PARALLEL_JOBS=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
                    echo "Using ${PARALLEL_JOBS} parallel compiler workers"
                    cmake --build "${CMAKE_BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${PARALLEL_JOBS}"
                '''
            }
        }

        stage('Run Unit Tests') {
            when {
                expression { return params.RUN_TESTS == true }
            }
            steps {
                echo "Executing unit test suite..."
                sh '''
                    ctest --test-dir "${CMAKE_BUILD_DIR}" \
                        --output-on-failure \
                        --output-junit ctest-report.xml \
                        -C "${BUILD_TYPE}"
                '''
            }
            post {
                always {
                    junit testResults: "${CMAKE_BUILD_DIR}/ctest-report.xml", allowEmptyResults: true
                }
            }
        }

        stage('Package Release') {
            when {
                expression { return params.PACKAGE_RELEASE == true || params.BUILD_TYPE == 'Release' }
            }
            steps {
                echo "Packaging installers with CPack..."
                sh '''
                    cd "${CMAKE_BUILD_DIR}"
                    cpack -C "${BUILD_TYPE}"
                    
                    if [ "$(uname)" = "Darwin" ]; then
                        PKG_FILE=$(find . -maxdepth 1 -name "*.pkg" -print -quit)
                        if [ -n "$PKG_FILE" ]; then
                            echo "Patching macOS package: $PKG_FILE..."
                            "../../scripts/packaging/patch_pkg.sh" "$PKG_FILE"
                        fi
                    fi
                '''
            }
        }
    }

    post {
        always {
            echo "Jenkins pipeline finished with status: ${currentBuild.currentResult}"
        }
        success {
            archiveArtifacts artifacts: """
                ${CMAKE_BUILD_DIR}/bin/stratawright,
                ${CMAKE_BUILD_DIR}/bin/daw-cli,
                ${CMAKE_BUILD_DIR}/*.pkg,
                ${CMAKE_BUILD_DIR}/*.dmg,
                ${CMAKE_BUILD_DIR}/*.exe,
                ${CMAKE_BUILD_DIR}/*.zip
            """, fingerprint: true, allowEmptyArchive: true
        }
        failure {
            echo "Build failed. Inspect stage logs above for compilation or test errors."
        }
    }
}

