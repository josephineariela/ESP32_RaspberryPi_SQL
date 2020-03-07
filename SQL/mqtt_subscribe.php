<?php

class phpMQTT {

	private $socket; 			/* holds the socket	*/
	private $msgid = 1;			/* counter for message id */
	public $keepalive = 10;		/* default keepalive timmer */
	public $timesinceping;		/* host unix time, used to detect disconects */
	public $topics = array(); 	/* used to store currently subscribed topics */
	public $debug = false;		/* should output debug messages */
	public $address;			/* broker address */
	public $port;				/* broker port */
	public $clientid;			/* client id sent to brocker */
	public $will;				/* stores the will of the client */
	private $username;			/* stores username */
	private $password;			/* stores password */

	public $cafile;

	function __construct($address, $port, $clientid, $cafile = NULL){
		$this->broker($address, $port, $clientid, $cafile);
	}

	/* sets the broker details */
	function broker($address, $port, $clientid, $cafile = NULL){
		$this->address = $address;
		$this->port = $port;
		$this->clientid = $clientid;
		$this->cafile = $cafile;
	}

	function connect_auto($clean = true, $will = NULL, $username = NULL, $password = NULL){
		while($this->connect($clean, $will, $username, $password)==false){
			sleep(10);
		}
		return true;
	}

	/* connects to the broker 
		inputs: $clean: should the client send a clean session flag */
	function connect($clean = true, $will = NULL, $username = NULL, $password = NULL){
		
		if($will) $this->will = $will;
		if($username) $this->username = $username;
		if($password) $this->password = $password;


		if ($this->cafile) {
			$socketContext = stream_context_create(["ssl" => [
				"verify_peer_name" => true,
				"cafile" => $this->cafile
				]]);
			$this->socket = stream_socket_client("tls://" . $this->address . ":" . $this->port, $errno, $errstr, 60, STREAM_CLIENT_CONNECT, $socketContext);
		} else {
			$this->socket = stream_socket_client("tcp://" . $this->address . ":" . $this->port, $errno, $errstr, 60, STREAM_CLIENT_CONNECT);
		}

		if (!$this->socket ) {
		    if($this->debug) error_log("stream_socket_create() $errno, $errstr \n");
			return false;
		}

		stream_set_timeout($this->socket, 5);
		stream_set_blocking($this->socket, 0);

		$i = 0;
		$buffer = "";

		$buffer .= chr(0x00); $i++;
		$buffer .= chr(0x06); $i++;
		$buffer .= chr(0x4d); $i++;
		$buffer .= chr(0x51); $i++;
		$buffer .= chr(0x49); $i++;
		$buffer .= chr(0x73); $i++;
		$buffer .= chr(0x64); $i++;
		$buffer .= chr(0x70); $i++;
		$buffer .= chr(0x03); $i++;

		//No Will
		$var = 0;
		if($clean) $var+=2;

		//Add will info to header
		if($this->will != NULL){
			$var += 4; // Set will flag
			$var += ($this->will['qos'] << 3); //Set will qos
			if($this->will['retain'])	$var += 32; //Set will retain
		}

		if($this->username != NULL) $var += 128;	//Add username to header
		if($this->password != NULL) $var += 64;	//Add password to header

		$buffer .= chr($var); $i++;

		//Keep alive
		$buffer .= chr($this->keepalive >> 8); $i++;
		$buffer .= chr($this->keepalive & 0xff); $i++;

		$buffer .= $this->strwritestring($this->clientid,$i);

		//Adding will to payload
		if($this->will != NULL){
			$buffer .= $this->strwritestring($this->will['topic'],$i);  
			$buffer .= $this->strwritestring($this->will['content'],$i);
		}

		if($this->username) $buffer .= $this->strwritestring($this->username,$i);
		if($this->password) $buffer .= $this->strwritestring($this->password,$i);

		$head = "  ";
		$head{0} = chr(0x10);
		$head{1} = chr($i);

		fwrite($this->socket, $head, 2);
		fwrite($this->socket,  $buffer);

	 	$string = $this->read(4);

		if(ord($string{0})>>4 == 2 && $string{3} == chr(0)){
			if($this->debug) echo "Connected to Broker\n"; 
		}else{	
			error_log(sprintf("Connection failed! (Error: 0x%02x 0x%02x)\n", 
			                        ord($string{0}),ord($string{3})));
			return false;
		}

		$this->timesinceping = time();

		return true;
	}

	/* read: reads in so many bytes */
	function read($int = 8192, $nb = false){

		//	print_r(socket_get_status($this->socket));
		
		$string="";
		$togo = $int;
		
		if($nb){
			return fread($this->socket, $togo);
		}
			
		while (!feof($this->socket) && $togo>0) {
			$fread = fread($this->socket, $togo);
			$string .= $fread;
			$togo = $int - strlen($string);
		}
		
	
		
		
			return $string;
	}

	/* subscribe: subscribes to topics */
	function subscribe($topics, $qos = 0){
		$i = 0;
		$buffer = "";
		$id = $this->msgid;
		$buffer .= chr($id >> 8);  $i++;
		$buffer .= chr($id % 256);  $i++;

		foreach($topics as $key => $topic){
			$buffer .= $this->strwritestring($key,$i);
			$buffer .= chr($topic["qos"]);  $i++;
			$this->topics[$key] = $topic; 
		}

		$cmd = 0x80;
		//$qos
		$cmd +=	($qos << 1);


		$head = chr($cmd);
		$head .= chr($i);
		
		fwrite($this->socket, $head, 2);
		fwrite($this->socket, $buffer, $i);
		$string = $this->read(2);
		
		$bytes = ord(substr($string,1,1));
		$string = $this->read($bytes);
	}

	/* ping: sends a keep alive ping */
	function ping(){
			$head = " ";
			$head = chr(0xc0);		
			$head .= chr(0x00);
			fwrite($this->socket, $head, 2);
			if($this->debug) echo "ping sent\n";
	}

	/* disconnect: sends a proper disconect cmd */
	function disconnect(){
			$head = " ";
			$head{0} = chr(0xe0);		
			$head{1} = chr(0x00);
			fwrite($this->socket, $head, 2);
	}

	/* close: sends a proper disconect, then closes the socket */
	function close(){
	 	$this->disconnect();
		stream_socket_shutdown($this->socket, STREAM_SHUT_WR);	
	}

	/* publish: publishes $content on a $topic */
	function publish($topic, $content, $qos = 0, $retain = 0){

		$i = 0;
		$buffer = "";

		$buffer .= $this->strwritestring($topic,$i);

		//$buffer .= $this->strwritestring($content,$i);

		if($qos){
			$id = $this->msgid++;
			$buffer .= chr($id >> 8);  $i++;
		 	$buffer .= chr($id % 256);  $i++;
		}

		$buffer .= $content;
		$i+=strlen($content);


		$head = " ";
		$cmd = 0x30;
		if($qos) $cmd += $qos << 1;
		if($retain) $cmd += 1;

		$head{0} = chr($cmd);		
		$head .= $this->setmsglength($i);

		fwrite($this->socket, $head, strlen($head));
		fwrite($this->socket, $buffer, $i);

	}

	/* message: processes a recieved topic */
	function message($msg){
		 	$tlen = (ord($msg{0})<<8) + ord($msg{1});
			$topic = substr($msg,2,$tlen);
			$msg = substr($msg,($tlen+2));
			$found = 0;
			foreach($this->topics as $key=>$top){
				if( preg_match("/^".str_replace("#",".*",
						str_replace("+","[^\/]*",
							str_replace("/","\/",
								str_replace("$",'\$',
									$key))))."$/",$topic) ){
					if(is_callable($top['function'])){
						call_user_func($top['function'],$topic,$msg);
						$found = 1;
					}
				}
			}

			if($this->debug && !$found) echo "msg recieved but no match in subscriptions\n";
	}

	/* proc: the processing loop for an "allways on" client 
		set true when you are doing other stuff in the loop good for watching something else at the same time */	
	function proc( $loop = true){

		if(1){
			$sockets = array($this->socket);
			$w = $e = NULL;
			$cmd = 0;
			
				//$byte = fgetc($this->socket);
			if(feof($this->socket)){
				if($this->debug) echo "eof receive going to reconnect for good measure\n";
				fclose($this->socket);
				$this->connect_auto(false);
				if(count($this->topics))
					$this->subscribe($this->topics);	
			}
			
			$byte = $this->read(1, true);
			
			if(!strlen($byte)){
				if($loop){
					usleep(100000);
				}
			 
			}else{ 
			
				$cmd = (int)(ord($byte)/16);
				if($this->debug) echo "Recevid: $cmd\n";

				$multiplier = 1; 
				$value = 0;
				do{
					$digit = ord($this->read(1));
					$value += ($digit & 127) * $multiplier; 
					$multiplier *= 128;
					}while (($digit & 128) != 0);

				if($this->debug) echo "Fetching: $value\n";
				
				if($value)
					$string = $this->read($value);
				
				if($cmd){
					switch($cmd){
						case 3:
							$this->message($string);
						break;
					}

					$this->timesinceping = time();
				}
			}

			if($this->timesinceping < (time() - $this->keepalive )){
				if($this->debug) echo "not found something so ping\n";
				$this->ping();	
			}
			

			if($this->timesinceping<(time()-($this->keepalive*2))){
				if($this->debug) echo "not seen a package in a while, disconnecting\n";
				fclose($this->socket);
				$this->connect_auto(false);
				if(count($this->topics))
					$this->subscribe($this->topics);
			}

		}
		return 1;
	}

	/* getmsglength: */
	function getmsglength(&$msg, &$i){

		$multiplier = 1; 
		$value = 0 ;
		do{
		  $digit = ord($msg{$i});
		  $value += ($digit & 127) * $multiplier; 
		  $multiplier *= 128;
		  $i++;
		}while (($digit & 128) != 0);

		return $value;
	}


	/* setmsglength: */
	function setmsglength($len){
		$string = "";
		do{
		  $digit = $len % 128;
		  $len = $len >> 7;
		  // if there are more digits to encode, set the top bit of this digit
		  if ( $len > 0 )
		    $digit = ($digit | 0x80);
		  $string .= chr($digit);
		}while ( $len > 0 );
		return $string;
	}

	/* strwritestring: writes a string to a buffer */
	function strwritestring($str, &$i){
		$ret = " ";
		$len = strlen($str);
		$msb = $len >> 8;
		$lsb = $len % 256;
		$ret = chr($msb);
		$ret .= chr($lsb);
		$ret .= $str;
		$i += ($len+2);
		return $ret;
	}

	function printstr($string){
		$strlen = strlen($string);
			for($j=0;$j<$strlen;$j++){
				$num = ord($string{$j});
				if($num > 31) 
					$chr = $string{$j}; else $chr = " ";
				printf("%4d: %08b : 0x%02x : %s \n",$j,$num,$num,$chr);
			}
	}
}

//PHPmyAdmin Initialization
$servername = "localhost";
$dbname = "Growth_Chamber";
$username = "root";
$password = "raspberry";
$api_key_value = "tPmAT5Ab3j7F9";

$mqtt_server 	= "tailor.cloudmqtt.com";    	// change if necessary
$mqtt_port 		= 14131;                     	// change if necessary
$mqtt_username 	= "hhaqsitb";                	// set your username
$mqtt_password 	= "MP7TFv0i040Q";            	// set your password
$client_id 	= "PHPPertama"; 				// make sure this is unique for connecting to sever - you could use uniqid()

function procmsg($topic, $msg){
	global $msg_OptTemp, $msg_OptHum, $msg_OptMoist, $msg_StartLight;
	global $msg_heater, $msg_cooler;
	if ($topic == 'OptTemp'){
		$msg_OptTemp = $msg;
	}elseif ($topic == 'OptHum'){
		$msg_OptHum = $msg;
	}elseif ($topic == 'OptMoist'){
		$msg_OptMoist = $msg;
	}elseif ($topic == 'StartLight'){
		$msg_StartLight = $msg;
	}elseif ($topic == 'heater'){
		$msg_heater = $msg;
	}elseif ($topic == 'cooler'){
		$msg_cooler = $msg;
	}elseif ($topic == 'humidifier'){
		$msg_humidifier = $msg;
	}elseif ($topic == 'dehumidifier'){
		$msg_dehumidifier = $msg;
	}elseif ($topic == 'moisture'){
		$msg_pump = $msg;
	}elseif ($topic == 'light'){
		$msg_light = $msg;
	}

	time.sleep(20);
}

$mqtt = new phpMQTT($mqtt_server, $mqtt_port, $client_id);

if(!$mqtt->connect(true, NULL, $mqtt_username, $mqtt_password)) {
	exit(1);
}else {
	$topic_OptTemp['OptTemp'] = array(
		"qos" => 0, 
		"function" => "procmsg"
	);	
	$topic_OptHum['OptHum'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_OptMoist['OptMoist'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_StartLight['StartLight'] = array(
		"qos" => 0, 
		"function" => "procmsg"
	);	

	$topic_heater['heater'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_cooler['cooler'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_humidifier['humidifier'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_dehumidifier['dehumidifier'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_pump['moisture'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	$topic_light['light'] = array(
		"qos" => 0, 
		"function" => "procmsg"
    );
	
	$mqtt->subscribe($topic_OptTemp, 0);
	$mqtt->subscribe($topic_OptHum, 0);
	$mqtt->subscribe($topic_OptMoist, 0);
	$mqtt->subscribe($topic_StartLight, 0);

	$mqtt->subscribe($topic_heater, 0);
	$mqtt->subscribe($topic_cooler, 0);
	$mqtt->subscribe($topic_humidifier, 0);
	$mqtt->subscribe($topic_dehumidifier, 0);
	$mqtt->subscribe($topic_pump, 0);
	$mqtt->subscribe($topic_light, 0);
	
	// if ($mqtt->proc(true)){
	while ($mqtt->proc()){

		echo "OptTemp = " . $msg_OptTemp . "<br>";
		echo "OptHum = " . $msg_OptHum . "<br>";
		echo "OptMoist = " . $msg_OptMoist . "<br>";
		echo "StartLight = " . $msg_StartLight . "<br>";
		echo "Heater = " . $msg_heater . "<br>";
		echo "Cooler = " . $msg_cooler . "<br>";
		echo "Humidifier = " . $msg_humidifier . "<br>";
		echo "Dehumidifier = " . $msg_dehumidifier . "<br>";
		echo "Pump = " . $msg_pump . "<br>";
		echo "Light = " . $msg_light . "<br>";

		// Create connection
		$conn = new mysqli($servername, $username, $password, $dbname);
		// Check connection
		if ($conn->connect_error) {
			die("Connection failed: " . $conn->connect_error);
		}	 
	
		$sqlOpt = "INSERT INTO OptimumData (temp, hum, moist, light) VALUES ('" . $msg_OptTemp . "', '" . $msg_OptHum . "', '" . $msg_OptMoist . "', '" . $msg_OptLight . "')";
	
		if ($conn->query($sqlOpt) === TRUE) {
			echo "Optimum Data created successfully";
		} 
		else {
			echo "Error: " . $sqlOpt . "<br>" . $conn->error;
		}

		echo "<br>";

		$sqlOpt = "DELETE FROM `OptimumData ` WHERE 1";

		$sqlMan = "INSERT INTO ManualData (heater, cooler, humidifier, dehumidifier, pump, light) VALUES ('" . $msg_heater . "', '" . $msg_cooler . "', '" . $msg_humidifier . "', '" . $msg_dehumidifier . "', '" . $msg_pump . "', '" . $msg_light . "')";
	
		if ($conn->query($sqlMan) === TRUE) {
			echo "Manual Data created successfully";
		} 
		else {
			echo "Error: " . $sqlMan . "<br>" . $conn->error;
		}

		echo "<br>";

		$conn->close();

	}
	
    $mqtt->close();
    
       
}
?>






