<?php
/*Program to get sensor data from ESP-S and save data to SQL Database
Created by: Josephine Ariella*/


//PHPmyAdmin Initialization
$servername = "localhost";
$dbname = "Growth_Chamber";
$username = "root";
$password = "raspberry";
$api_key_value = "tPmAT5Ab3j7F9";

$api_key= $temp = $hum = "";

//Getting data from ESP32-S
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $api_key = test_input($_POST["api_key"]);
    if($api_key == $api_key_value) {
        $temp = test_input($_POST["temp"]);
        $hum = test_input($_POST["hum"]);
        $moist = test_input($_POST["moist"]);
        $light = test_input($_POST["light"]);
        // Create connection
        $conn = new mysqli($servername, $username, $password, $dbname);
        // Check connection
        if ($conn->connect_error) {
            die("Connection failed: " . $conn->connect_error);
        } 
        
        $sql = "INSERT INTO ActualData (temp, hum, moist, light) VALUES ('" . $temp . "', '" . $hum . "', '" . $moist . "', '" . $light . "')";
        
        if ($conn->query($sql) === TRUE) {
            echo "New record created successfully";
        } 
        else {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }
        $conn->close();
    }
    else {
        echo "Wrong API Key provided.";
    }

}
else {
    // Create connection
    $conn = new mysqli($servername, $username, $password, $dbname);
    // Check connection
    if ($conn->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }   

    //Ambil data dari tabel database SensorData
    $sql = "SELECT id, temp, hum, moist, light, reading_time FROM ActualData ORDER BY id DESC";

    echo '<table cellspacing="5" cellpadding="5">
      <tr> 
        <td>ID</td> 
        <td>Temp</td> 
        <td>Hum</td> 
        <td>Moist</td> 
        <td>Light</td> 
        <td>Timestamp</td> 
      </tr>';
 
    if ($result = $conn->query($sql)) {
        while ($row = $result->fetch_assoc()) {
            $row_id = $row["id"];
            $row_temp = $row["temp"];
            $row_hum = $row["hum"];  
            $row_moist = $row["moist"]; 
            $row_light = $row["light"];  
            $row_reading_time = $row["reading_time"];

            echo '<tr> 
                <td>' . $row_id . '</td> 
                <td>' . $row_temp . '</td> 
                <td>' . $row_hum . '</td> 
                <td>' . $row_moist . '</td> 
                <td>' . $row_light . '</td> 
                <td>' . $row_reading_time . '</td> 
              </tr>';
        }
        $result->free();
    }

    $conn->close();
    

function test_input($data) {
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}

?>
