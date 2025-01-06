<?php
    include_once("utils/dbconfig.php");
    include_once("utils/util.php");
    include_once("student_delivery.php");
    session_start();

    if ($_SERVER["REQUEST_METHOD"] == "POST" && 
        isset($_POST["username"]) && 
        isset($_POST["password"]))
    {
        $username = query_db_login($_POST["username"], $_POST["password"]);
        if (null != $username)
        {
            $_SESSION['cookie'] = $username;
            create_user_folder("users/" . $_SESSION['cookie']);
            header("location: index.php?language=en");
        }
        else 
        {
            echo "Invalid username or password!";
        }
    }
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <title>Login</title>
</head>
<body>
    <div class="container">
        <h1>Sign in</h1>
        <form action="login.php" method="POST">
            <label for="username">Username</label>
            <input type="text" name="username" id="username">

            <label for="password">Password</label>
            <input type="password" name="password" id="password">            
            
            <button type="submit">Login</button>
        </form>
    </div>
</body>
</html>