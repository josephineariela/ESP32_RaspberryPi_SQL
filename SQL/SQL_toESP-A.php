<?php
/*Program to send data from SQL Database to ESP-A
Created by: Josephine Ariella*/

$api_key_value = "tPmAT5Ab3j7F9";

$servername = "localhost";

//PHPmyAdmin Initialization
$servername = "localhost";
$dbname = "Growth_Chamber";
$username = "root";
$password = "raspberry";
$api_key_value = "192001003";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}   


//Ambil data dari tabel ActualData
$sql = "SELECT id, temp, hum, moist, light, reading_time FROM ActualData ORDER BY id DESC";


if ($result = $conn->query($sql)) {
    if($row = $result->fetch_assoc()) {
            $ActTemp = $row["temp"];
            $ActHum = $row["hum"];
            $ActMoist = $row["moist"];
            $ActLight = $row["light"];
            echo $ActTemp;
            echo "v";
            echo $ActHum;
            echo "w";
            echo $ActMoist;
            echo "x";
            echo $ActLight;
            echo "y";
    }
    $result->free();
}

// //Ambil data dari tabel OptimumData
// $sql = "SELECT id, temp, hum, moist, light, reading_time FROM OptimumData ORDER BY id DESC";


// if ($result = $conn->query($sql)) {
//     if($row = $result->fetch_assoc()) {
//             $OptTemp = $row["temp"];
//             $OptHum = $row["hum"];
//             $OptMoist = $row["moist"];
//             $OptLight = $row["light"];
//             echo $OptTemp;
//             echo "v";
//             echo $OptHum;
//             echo "w";
//             echo $OptMoist;
//             echo "x";
//             echo $OptLight;
//             echo "y";
//     }
//     $result->free();
// }

$conn->close();


//mysqli_query($dbconnect, $sql);

?>