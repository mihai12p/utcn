<?php
    include_once("utils/dbconfig.php");
    include_once("utils/util.php");
    include_once("student_delivery.php");
    session_start();
    if (!isset($_SESSION["cookie"]))
    {
        header("location: login.php");
    }

    $language_path = get_language_php("en");
    if (isset($_GET["language"]))
    {
        $temp_language_path = get_language_php($_GET["language"]);
        if ($temp_language_path != null)
        {
            $language_path = $temp_language_path;
        }
        else
        {
            echo "Language " . $_GET["language"] . " not supported. Defaulting to English.";
        }
    }
    include_once($language_path);

    /* Dispatcher for POST calls */
    if ($_SERVER["REQUEST_METHOD"] == "POST") {
        if (isset($_POST['add_message_for_user']) && 
            isset($_POST["message"]))
        {
            add_message_for_user($_SESSION["cookie"], $_POST["message"]);
        }
        else if (isset($_POST['add_photo_for_user']) && 
            isset($_FILES["photo"]))
        {
            if (false == is_valid_image($_FILES["photo"]["tmp_name"]))
            {
                echo "Invalid image submitted!";
            }
            else
            {
                $file_userphoto = copy_file_to_userfolder($_SESSION["cookie"], $_FILES["photo"]);
                add_photo_path_to_user($_SESSION["cookie"], $file_userphoto);
                echo "Added image " . $_FILES["photo"]["name"] . " successfully!";
            }
        }
        else if (isset($_POST['add_memo_for_user']) && 
            isset($_FILES["memo"]))
        {
            copy_file_to_userfolder($_SESSION["cookie"], $_FILES["memo"]);
            echo "Added memo " . $_FILES["memo"]["name"] . " successfully!";
        }
        else if (isset($_POST['view_memo_for_user']) && 
            isset($_POST["memo"]))
        {
            $memo_output = get_memo_content_for_user($_SESSION["cookie"], $_POST["memo"]);
        }
    }
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <title><?php echo sprintf($welcome_msg, $_SESSION["cookie"]); ?></title>
</head>
<body>
    <div class="container">
        <table>
            <tr>
                <td>
                    <?php 
                        $path = get_photo_path_for_user($_SESSION['cookie']); 
                        if ($path != null)
                        {
                            echo "<img src='" . $path . "' width=50 height=50></img>";
                        }
                    ?>
                </td>
                <td>
                    <h2>&nbsp;<?php echo sprintf($welcome_msg, $_SESSION["cookie"]); ?></h2>
                </td>
            </tr>
        </table>
        <hr/>
        <table>
            <tr>
                <!-- Form tasked with adding the user message -->
                <form action="index.php" method="POST">
                    <td><label><?php echo $add_comment ?></label></td>
                    <td><input type="text" name="message" id="message"></td>
                    <td><button type="submit" name="add_message_for_user"><?php echo $add_btn ?></button></td>
                </form>
            </tr>
            <tr>
                <!-- Form tasked with adding the user photo -->
                <form action="index.php" method="POST" enctype="multipart/form-data">
                    <td><label><?php echo $add_photo ?></label></td>
                    <td><input type="file" name="photo" id="photo"/></td>
                    <td><button type="submit" name="add_photo_for_user"><?php echo $add_btn ?></button></td>
                </form>
            </tr>
        </table>
        <hr/>
        <table>
            <tr>
                <!-- Form tasked with adding one user memo -->
                <form action="index.php" method="POST" enctype="multipart/form-data">
                    <td><label><?php echo $add_memo ?></label></td>
                    <td><input type="file" name="memo" id="memo"/></td>
                    <td><button type="submit" name="add_memo_for_user"><?php echo $add_btn ?></button></td>
                </form>
            </tr>
            <tr>
                <!-- Form tasked with viewing one user memo -->
                <form action="index.php" method="POST">
                    <td><label><?php echo $view_memo ?></label></td>
                    <td><input type="text" name="memo" id="memo"></td>
                    <td><button type="submit" name="view_memo_for_user"><?php echo $view_btn ?></button></td>
                </form>
            </tr>
        </table>

        <?php if (isset($memo_output)): ?>
            <hr/>
            <p><?php echo $memo_output ?></p>
        <?php endif; ?>

        <hr/>

        <form action="logout.php" method="POST">
            <button type="submit"><?php echo $logout ?></button>
        </form>

        <br/>   
        <?php echo $livechat ?>
        <table>
            <th><?php echo $username_msg ?></th>
            <th><?php echo $message_msg ?></th>
            <?php foreach (get_message_rows() as $row): ?>
                <tr>
                    <td><?php echo $row['username'] ?></td>
                    <td><?php echo $row['message'] ?></td>
                </tr>
            <?php endforeach; ?>
        </table>
    </div>
</body>
</html>