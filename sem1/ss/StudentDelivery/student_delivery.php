<?php
    /* 
    IMPORTANT - PLEASE READ ME
        This is the ONLY file that I will use to validate your solution's implementation. Please keep in mind that only the changes done to this file
        will be tested, and if you modify anything in any other files those changes won't be taken in account when I validate your solution.
        Also, please do not rename the file.

        In a separate file (named answers.txt) answer the following questions for each function you implement:
            * What vulnerabilities can there be in that function 
                (take in account the fact that the function may not be vulnerable and explicitly say so if you consider it to be that way)
            * What specific mitigation you used for each of the vulnerabilities listed above
        
        For the function named 'get_language_php' which is already implemented make sure to answer and do all the steps required that are listed
        above the implementation.

    DELIVERY REQUIREMENTS
        When delivering your solution, please ensure that you create a .zip archive file (make sure it's zip, not 7z, rar, winzip, etc)
        with the name "LastnameFirstname.zip" (for example MunteaAndrei.zip or RatiuRazvan.zip) and in the root of the zip file please 
        add the student_delivery.php file modified by you (keep the name as it is) and answers.txt file where you answered the questions.
    */

    /* Implement query_db_login - this function is used in login.php */
    /* 
        Description - Must query the database to obtain the username that matches the 
        input parameters ($username, $password), or must return null if there is no match.
        The password is stored as MD5, so the query must convert the password received as parameter to
        MD5 and AFTER that interogate the DB with the MD5.
        PARAMETERS:
            $username: username field from post request
            $password: password field from post request
        MUST RETURN:
            null - if user credentials are not correct
            username - if credentials match a user
    */
    function query_db_login($username, $password) 
    {
        $conn = get_mysqli();
        $found = null;
        
        if ($stmt = $conn->prepare("SELECT username FROM users WHERE username = ? AND password = MD5(?)"))
        {
            $stmt->bind_param("ss", $username, $password);
            $stmt->execute();
            $result = $stmt->get_result();
            
            if ($result->num_rows === 1)
            {
                $row = $result->fetch_assoc();
                $found = $row['username'];
            }
            
            $stmt->close();
        }
        
        $conn->close();
        return $found;
    }

    /* Implement get_message_rows - this function is used in index.php */
    /* 
        Function must query the db and fetch all the entries from the 'messages' table
        (username, message - see MUST RETURN for more details) and return them in a separate array, 
        or return an empty array if there are no entries.
        PARAMETERS:
            No parameters
        MUST RETURN:
            array() - containing each of the rows returned by mysqli if there is at least one message
                      (code will use both $results['username'] and $results['message'] to display the data)
            empty array() - if there are NO messages
    */
    function get_message_rows() 
    {
        $conn = get_mysqli();
        $results = array();
        
        if ($stmt = $conn->prepare("SELECT username, message FROM messages ORDER BY username"))
        {
            $stmt->execute();
            $result = $stmt->get_result();
            
            while ($row = $result->fetch_assoc())
            {
                $row['message'] = htmlspecialchars($row['message'], ENT_QUOTES, 'UTF-8');
                $results[] = $row;
            }
            
            $stmt->close();
        }
        
        $conn->close();
        return $results;
    }

    /* Implement add_message_for_user - this function is used in index.php */
    /* 
        Function must add the message received as parameter to the database's 'message' table.
        PARAMETERS:
            $username - username for the user submitting the message
            $message - message that the user wants to submit
        MUST RETURN:
            Return is irrelevant here
    */
    function add_message_for_user($username, $message) 
    {
        $username = htmlspecialchars($username, ENT_QUOTES, 'UTF-8');
        $message = htmlspecialchars($message, ENT_QUOTES, 'UTF-8');

        if (mb_strlen($message, 'UTF-8') === 0)
        {
            return "Message cannot be empty.";
        }

        $conn = get_mysqli();

        if (mb_strlen($message, 'UTF-8') > 256)
        {
            $message = mb_substr($message, 0, 256, 'UTF-8');
        }
        
        if ($stmt = $conn->prepare("INSERT INTO messages (username, message) VALUES (?, ?)"))
        {
            $stmt->bind_param("ss", $username, $message);
            $stmt->execute();
            $stmt->close();
        }
        
        $conn->close();
    }

    /* Implement is_valid_image - this function is used in index.php */
    /* 
        This function will validate if the file contained at $image_path is indeed an image.
        PARAMETERS:
            $image_path: path towards the file on disk
        MUST RETURN:
            true - file is an image
            false - file is not an image
    */
    function is_valid_image($image_path) 
    {        
        if (!file_exists($image_path))
        {
            return false;
        }

        $finfo = finfo_open(FILEINFO_MIME_TYPE);
        $mime_type = finfo_file($finfo, $image_path);
        finfo_close($finfo);
        
        $allowed_types = array(
            'image/jpeg',
            'image/png',
            'image/gif'
        );

        if (!in_array($mime_type, $allowed_types))
        {
            return false;
        }
        
        return true;
    }

    /* Implement add_photo_to_user - this function is used in index.php */
    /* 
        This function must update the 'users' table and set the 'file_userphoto' field with 
        the value given to the $file_userphoto parameter
        PARAMETERS:
            $username - user for which to update the row
            $file_userphoto - value to be put in the 'file_userphoto' column (a path to an image)
        MUST RETURN:
            Return is irrelevant here
    */
    function add_photo_path_to_user($username, $file_userphoto) 
    {
        $file_userphoto = htmlspecialchars($file_userphoto, ENT_QUOTES, 'UTF-8');

        $user_folder = "users/" . $username . "/";
        $real_path = realpath($user_folder . $file_userphoto);
    
        if ($real_path === false || strpos($real_path, $user_folder) !== 0)
        {
            return "Invalid file path!";
        }

        $conn = get_mysqli();

        if ($stmt = $conn->prepare("UPDATE users SET file_userphoto = ? WHERE username = ?"))
        {
            $stmt->bind_param("ss", $file_userphoto, $username);
            $stmt->execute();
            $stmt->close();
        }
        
        $conn->close();
    }

    /* Implement get_photo_path_for_user - this function is used in index.php */
    /* 
        This function must obtain from the 'users' table the field named file_userphoto and
        return is as a string. If there is nothing in the database, then return null.
        PARAMETERS:
            $username - user for which to query the file_userphoto column
        MUST RETURN:
            string - string containing the value from the DB, if there is such a value
            null - if there is no value in the DB
    */
    function get_photo_path_for_user($username) 
    {
        $conn = get_mysqli();
        $path = null;

        if ($stmt = $conn->prepare("SELECT file_userphoto FROM users WHERE username = ?"))
        {
            $stmt->bind_param("s", $username);
            $stmt->execute();
            $result = $stmt->get_result();
            
            if ($result->num_rows === 1)
            {
                $row = $result->fetch_assoc();
                $path = $row['file_userphoto'];
            }
            
            $stmt->close();
        }
        
        $conn->close();
        return $path;
    }

    /* Implement get_memo_content_for_user - this function is used in index.php */
    /* 
        This function must open the memo file for the current user from it's folder and return its content as a string.
        If the memo does not exist, the function must return the string "No such file!".
        PARAMETERS:
            $username - user for which obtain the memo file
            $memoname - the name of the memo the user requested to see
        MUST RETURN:
            string containing the data from the memo file (it's content)
            "No such file!" if there's no such file.
    */
    function get_memo_content_for_user($username, $memoname) 
    {
        $username = basename($username);
        $memoname = basename($memoname);

        $memo_path = "users/" . $username . "/" . $memoname;

        $real_path = realpath($memo_path);
        $user_dir = realpath("users/" . $username);
        
        if ($real_path === false ||
            $user_dir === false || 
            strpos($real_path, $user_dir) !== 0)
        {
            return "No such file!";
        }
        
        if (!file_exists($memo_path) || !is_readable($memo_path))
        {
            return "No such file!";
        }

        $content = file_get_contents($memo_path);
        if ($content === false)
        {
            return "No such file!";
        }

        return htmlspecialchars($content, ENT_QUOTES, 'UTF-8');
    }

    /* 
        Evaluate the impact of 'get_language_php' by explaining what are the risks of this function's default implementation
        (the one you received) by answering the following questions:
            - What is the vulnerability present in this function?
            - What other vulnerability can be chained with this vulnerability to inflict damage on the web application and where is it present?
            - What can the attacker do once he chains the two vulnerabilities?
        After that, modify the get_language_php function to no longer present a security risk.
        This function is used in index.php
    */
    /*
        This function must return the path to the language file corresponding to the desired language or null if the file
        does not exist. All language files must be in the language folder or else they are not supported.
        PARAMETERS:
            $language - desired language (e.g en)
        MUST RETURN:
            path to the en language file (languages/en.php)
            null if the language is not supported
    */
    function get_language_php($language)
    {
        $allowed_languages = array('en', 'ro');
        
        $language = strtolower(trim($language));
        
        if (!in_array($language, $allowed_languages))
        {
            return null;
        }

        $language_path = realpath("language/" . $language . ".php");
        $language_dir = realpath("language");
        
        if ($language_path === false || 
            $language_dir === false || 
            strpos($language_path, $language_dir) !== 0 || 
            !is_file($language_path))
        {
            return null;
        }
        
        return $language_path;
    }
?>