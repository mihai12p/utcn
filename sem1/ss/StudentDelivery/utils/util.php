<?php
    function copy_file_to_userfolder($username, $file)
    {
        $source = $file["tmp_name"];
        $destination = "users/" . $_SESSION["cookie"] . "/" . $file["name"];
        
        copy($file["tmp_name"], $destination); 

        return $destination;
    }

    function create_user_folder($user_folder)
    {
        $abs_path = realpath($user_folder);
        if ($abs_path != false AND is_dir($abs_path))
        {
            return;
        }

        if (is_file($user_folder))
        {
            die("Framework error: file '" . $user_folder . "' encountered while trying to create folder named " . $user_folder);
            return;
        }

        // Beware - this function creates the folder with 0777 permissions (maximum permissions). 
        // We use it here like this because it's easy but don't do this in a production environment.
        mkdir($user_folder);    
    }
?>