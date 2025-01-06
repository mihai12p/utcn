<?php 
    $server = "localhost";
    $username = "root";
    $password = "";
    $dbname = "student_delivery";

    function get_mysqli() {
        global $server, $username, $password, $dbname;

        $conn = new mysqli($server, $username, $password, $dbname);
        if ($conn->connect_error) {
            die("Could not get connection to mysqli " . $conn->connect_error);
        }

        return $conn;
    }
?>