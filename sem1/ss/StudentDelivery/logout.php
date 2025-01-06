<?php
    session_start();

    if ($_SERVER["REQUEST_METHOD"] == "POST") {
        session_unset();
    }
    header('location: login.php');
?>