#include "test_includes.hpp"


/**
 * @brief	In this file you can add unit tests for the functionality.
 *          Only for unit tests here you are free to use c++ and any std library.
 */
namespace SafeStorageUnitTests
{

TEST_CLASS(UserActivityTest)
{
    TEST_METHOD_INITIALIZE(UserActivityTestInit)
    {
        // Ensure we cleanup any pre-existing files.
        if (std::filesystem::is_regular_file(".\\users.txt"))
        {
            std::filesystem::remove(".\\users.txt");
        }
        if (std::filesystem::is_directory(".\\users"))
        {
            std::filesystem::remove_all(".\\users");
        }

        Assert::IsTrue(NT_SUCCESS(SafeStorageInit()));
    }

    TEST_METHOD_CLEANUP(UserActivityTestCleanup)
    {
        SafeStorageDeinit();
    }

    TEST_METHOD(UserRegisterLoginLogout)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username[] = "UserA";
        const char password[] = "PassWord1@";

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        //
        // As per requirements.
        // Registering a user requires the creation of the following:
        //  <current dir>           - %appdir% (application directory)
        //       |- users.txt       (file)
        //       |- users           (directory)
        //           |- UserA       (directory)
        //
        Assert::IsTrue(std::filesystem::is_regular_file(".\\users.txt"));
        Assert::IsTrue(std::filesystem::is_directory(".\\users"));
        Assert::IsTrue(std::filesystem::is_directory(".\\users\\UserA"));

        status = SafeStorageHandleLogin(username,
                                        static_cast<uint16_t>(strlen(username)),
                                        password,
                                        static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogout();
        Assert::IsTrue(NT_SUCCESS(status));
    };

    TEST_METHOD(FileTransfer)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username[] = "UserB";
        const char password[] = "PassWord1@";

        const char submissionName[] = "Homework";
        const char submissionFilePath[] = ".\\dummyData";

        // Drop dummy data for transfer test
        {
            std::ofstream transferFileTest(submissionFilePath);
            transferFileTest << "This is a dummy content";
        }

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogin(username,
                                        static_cast<uint16_t>(strlen(username)),
                                        password,
                                        static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleStore(submissionName,
                                        static_cast<uint16_t>(strlen(submissionName)),
                                        submissionFilePath,
                                        static_cast<uint16_t>(strlen(submissionFilePath)));
        Assert::IsTrue(NT_SUCCESS(status));

        //
        // As per requirements; a file called "Homework"
        // must be created under the users directory.
        //
        // The file must have the same content as the copied file.
        //
        Assert::IsTrue(std::filesystem::is_regular_file(".\\users\\UserB\\Homework"));
        Assert::IsTrue(std::filesystem::file_size(submissionFilePath) ==
                       std::filesystem::file_size(".\\users\\UserB\\Homework"));
        status = SafeStorageHandleRetrieve(submissionName,
                                           static_cast<uint16_t>(strlen(submissionName)),
                                           submissionFilePath,
                                           static_cast<uint16_t>(strlen(submissionFilePath)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogout();
        Assert::IsTrue(NT_SUCCESS(status));
    };

    TEST_METHOD(InvalidUserRegistration)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        const char invalidUsername1[] = "User123";
        const char password[] = "PassWord1@";
        status = SafeStorageHandleRegister(invalidUsername1,
                                           static_cast<uint16_t>(strlen(invalidUsername1)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsFalse(NT_SUCCESS(status));

        const char validUsername[] = "UserC";
        const char invalidPassword1[] = "Password123";
        status = SafeStorageHandleRegister(validUsername,
                                           static_cast<uint16_t>(strlen(validUsername)),
                                           invalidPassword1,
                                           static_cast<uint16_t>(strlen(invalidPassword1)));
        Assert::IsFalse(NT_SUCCESS(status));

        const char invalidUsername2[] = "Usr";
        status = SafeStorageHandleRegister(invalidUsername2,
                                           static_cast<uint16_t>(strlen(invalidUsername2)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsFalse(NT_SUCCESS(status));

        const char shortPassword[] = "P1@";
        status = SafeStorageHandleRegister(validUsername,
                                           static_cast<uint16_t>(strlen(validUsername)),
                                           shortPassword,
                                           static_cast<uint16_t>(strlen(shortPassword)));
        Assert::IsFalse(NT_SUCCESS(status));
    };

    TEST_METHOD(RepeatedUserRegistration)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username[] = "UserD";
        const char password[] = "PassWord1@";

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        std::string userDirectory = std::string(".\\users\\") + username;
        Assert::IsTrue(std::filesystem::is_directory(userDirectory));

        std::ifstream userFile(".\\users.txt");
        std::string line;
        bool userFound = false;
        while (std::getline(userFile, line))
        {
            if (line.find(username) != std::string::npos)
            {
                userFound = true;
                break;
            }
        }
        userFile.close();
        Assert::IsTrue(userFound);

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsFalse(NT_SUCCESS(status));
    };

    TEST_METHOD(ConcurrentLogins)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username1[] = "UserE";
        const char password1[] = "PassWord1@";

        const char username2[] = "UserF";
        const char password2[] = "PassWord2#";

        status = SafeStorageHandleRegister(username1,
                                           static_cast<uint16_t>(strlen(username1)),
                                           password1,
                                           static_cast<uint16_t>(strlen(password1)));
        Assert::IsTrue(NT_SUCCESS(status));
        Assert::IsTrue(std::filesystem::is_directory(std::string(".\\users\\") + username1));

        status = SafeStorageHandleRegister(username2,
                                           static_cast<uint16_t>(strlen(username2)),
                                           password2,
                                           static_cast<uint16_t>(strlen(password2)));
        Assert::IsTrue(NT_SUCCESS(status));
        Assert::IsTrue(std::filesystem::is_directory(std::string(".\\users\\") + username2));

        status = SafeStorageHandleLogin(username1,
                                        static_cast<uint16_t>(strlen(username1)),
                                        password1,
                                        static_cast<uint16_t>(strlen(password1)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogin(username2,
                                        static_cast<uint16_t>(strlen(username2)),
                                        password2,
                                        static_cast<uint16_t>(strlen(password2)));
        Assert::IsFalse(NT_SUCCESS(status));

        status = SafeStorageHandleLogout();
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogin(username2,
                                        static_cast<uint16_t>(strlen(username2)),
                                        password2,
                                        static_cast<uint16_t>(strlen(password2)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogout();
        Assert::IsTrue(NT_SUCCESS(status));
    };

    TEST_METHOD(SecurePasswordStorage)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username[] = "UserG";
        const char password[] = "PassWord1@";

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        std::string userDirectory = std::string(".\\users\\") + username;
        Assert::IsTrue(std::filesystem::is_directory(userDirectory));

        std::ifstream userFile(".\\users.txt");
        std::string line;
        bool plainPasswordFound = false;
        while (std::getline(userFile, line))
        {
            if (line.find(password) != std::string::npos)
            {
                plainPasswordFound = true;
                break;
            }
        }
        userFile.close();
        Assert::IsFalse(plainPasswordFound);
    };

    TEST_METHOD(BruteForceProtection)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username[] = "UserH";
        const char password[] = "PassWord1@";
        const char wrongPassword[] = "WrongPass1#";

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        for (BYTE i = 0; i < 10; ++i)
        {
            status = SafeStorageHandleLogin(username,
                                            static_cast<uint16_t>(strlen(username)),
                                            wrongPassword,
                                            static_cast<uint16_t>(strlen(wrongPassword)));
        }
        Assert::IsFalse(NT_SUCCESS(status));

        status = SafeStorageHandleLogin(username,
                                        static_cast<uint16_t>(strlen(username)),
                                        password,
                                        static_cast<uint16_t>(strlen(password)));
        Assert::IsFalse(NT_SUCCESS(status));
    };

    TEST_METHOD(PathTraversal)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        const char username[] = "UserI";
        const char password[] = "PassWord1@";

        const char submissionName[] = "..\\Homework";
        const char submissionFilePath[] = ".\\dummyData";

        // Drop dummy data for transfer test
        {
            std::ofstream transferFileTest(submissionFilePath);
            transferFileTest << "This is a dummy content";
        }

        status = SafeStorageHandleRegister(username,
                                           static_cast<uint16_t>(strlen(username)),
                                           password,
                                           static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleLogin(username,
                                        static_cast<uint16_t>(strlen(username)),
                                        password,
                                        static_cast<uint16_t>(strlen(password)));
        Assert::IsTrue(NT_SUCCESS(status));

        status = SafeStorageHandleStore(submissionName,
                                        static_cast<uint16_t>(strlen(submissionName)),
                                        submissionFilePath,
                                        static_cast<uint16_t>(strlen(submissionFilePath)));
        Assert::IsFalse(NT_SUCCESS(status));

        Assert::IsFalse(std::filesystem::is_regular_file(".\\users\\UserI\\Homework"));
        status = SafeStorageHandleRetrieve(submissionName,
                                           static_cast<uint16_t>(strlen(submissionName)),
                                           submissionFilePath,
                                           static_cast<uint16_t>(strlen(submissionFilePath)));
        Assert::IsFalse(NT_SUCCESS(status));

        status = SafeStorageHandleLogout();
        Assert::IsTrue(NT_SUCCESS(status));
    };
};

};