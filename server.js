const { MongoClient, Double } = require('mongodb');	// require the mongodb driver
//test
const express = require("express");
const multer = require("multer");
const fs = require("fs");
const path = require('path');
const {exec, spawn} = require("child_process");
const mv = require('mv');
const crypto = require('crypto');
const bodyParser = require('body-parser');
const sharp = require('sharp');

sharp.cache(false);

// This keeps track of every user within 10m of every scooter
let scooter_users = new Map();

const uri = 'mongodb://20.238.80.71:2541'
// const uri = 'mongodb://127.0.0.1:27017';
const client = new MongoClient(uri);


const app = express();
const port = 8081;
const host = '20.238.80.71';
//const host = '127.0.0.1';

var jsonParser = bodyParser.json();
var urlParser = bodyParser.urlencoded({ extended: true });


app.use(jsonParser);
//app.use(methodOverride('_method'));


// app.get('/', (req, res) => {
//     res.send('VisiRide Server running!')
// })



// returns all users 
app.get('/users', async (req, res) => {
    try {
        const result =  await client.db('VisiRide').collection('users').find().sort({ 'rating' : -1 }).toArray();
		let users =[];
		for(let i = 0; i < result.length; i++){
			let user = {
				"username" : result[i].username,
				"password" : result[i].password,
				"lat" : result[i].deviceLocation.lat,
				"lon" : result[i].deviceLocation.lon
				// "time" : result[i].time

                // "image" : result[i].image
			}
			users.push(user);
		}
		
        res.status(200).send(users);
    } 
    catch(err) {
        console.log(err);
        res.send(400).send('No Users!');
    }
})




const faceStorageEngine = multer.diskStorage({
	destination: (req,file, cb) =>{
		cb(null, "./known_faces");
	},

	filename: (req, file, cb) =>{
		// Need to change the name in case two or more files share a name
		var fname = file.originalname; // This is temporary
		cb(null, fname);
	},
});


const add_face = multer({storage: faceStorageEngine});


app.route('/sign_up').post(jsonParser, async (req, res) => {
    try {
		
        var username =  req.body.username;
		var password =  req.body.password;

		var lat = parseFloat(req.body.lat);
		var lon = parseFloat(req.body.lon);
		var image = req.body.image;

		if(isValid(username) && await isTaken(username) == false){

			let folder_name = "./" + username;
			if (!fs.existsSync(folder_name)) {
				fs.mkdirSync(folder_name, (err) => {
					if (err) 
						console.error(err);
				});
			}
			
			fs.writeFile(folder_name + "/" + username + ".jpg", image, {encoding: 'base64'},()=>{
				exec("face_detection " +  folder_name, async (err, stdout, stderr) =>{
					if(err) console.log(err);
					else{
						if(stdout === ""){
							fs.unlink(folder_name + "/" + username + ".jpg",(err)=>{
								if(err) throw err;
							});
							res.status(200).send('Please provide a valid photo.');
						} else {
							password = encrypt(password);
							// up to 16MB in size
							let user =  {
								"username" : username,
								"password" : password,
								"deviceLocation" : {
									"lat" : lat,
									"lon" : lon,
								},
								"time" : 0,
								"image" : image
							}

							await client.db('VisiRide').collection('users').insertOne(user);
							fs.rm(folder_name, { recursive: true }, err => {
								if (err) {
								  throw err
								}
							  
								console.log(`${folder_name} is deleted!`)
							  })
							//res.redirect('/'); // homepage
							res.status(200).send('New User Added!');
						}
					}
				});
			});
		} else{
			res.status(200).send('Please enter a valid username.');
		}
    } 
    catch(err){
        console.log(err);
        res.status(400).send(err);
    }
});




app.route("/sign_in").get(async (req,res) =>{
	try{
		let username = req.query.username;
		let password = req.query.password;

		let users = await client.db('VisiRide').collection('users').find({username : username}).toArray();

		if( users.length === 1 && isCorrectPassword(password,users[0].password)) {
			var userImg = users[0].image;	//sending user image if password authentication is successful 
			//res.redirect('/'); // homepage
			res.status(200).send(userImg);
		} else {
			//res.redirect("/login");
			res.status(200).send("Incorrect username or password");
		}
	}catch(err){
		console.log(err);
        res.status(400).send(err);
	}

});




app.post('/add_scooter', async (req, res) => {
    try {
		let scooter = {
			"scooterId": req.query.scooterId,
			"user": "_none",
            "currentCharge": req.query.charge,
            "location" : {
                "lat" : parseFloat(req.query.lat),
			    "lon" : parseFloat(req.query.lon)   
            }	
		}
        await client.db('VisiRide').collection('scooters').insertOne(scooter);
		fs.mkdir("./scooterCloseUsers/" + req.query.scooterId, err => {
			if (err) throw err
			console.log('Directory is created.');
		});
		fs.mkdir("./scooterFacesJPG/" + req.query.scooterId, err => {
			if (err) throw err
			console.log('Directory is created.');
		});

		fs.open('./scooterFaces64/' + req.query.scooterId + '.txt', 'w', (err, file) => {
			if (err) throw err
			console.log('File is created.')
		});

        res.status(200).send('New Scooter Added!');
    } 
    catch(err){
        console.log(err);
        res.status(400).send(err);
    }
})


app.get("/scooters", async (req, res) =>{
	try {
        const result =  await client.db('VisiRide').collection('scooters').find().sort({ 'rating' : -1 }).toArray();
        res.status(200).send(result);
    } 
    catch(err) {
        console.log(err);
        res.status(400).send('No Scooters!');
    }
})

// This should happen periodically
app.post("/update_user_location", async (req, res) =>{      
	try{
		var user_name = req.body.username;

		var lat = parseFloat(req.body.lat);
		var lon = parseFloat(req.body.lon);

		// update the location of the user
		const user = await client.db('VisiRide').collection('users').updateOne({ username : user_name }, 
			{ $set:{ deviceLocation : { lat: lat, lon: lon } }});

        console.log("user: " + user_name);

		const scooters =  await client.db('VisiRide').collection('scooters').find().sort({ 'rating' : -1 }).toArray();

		// check if the user is within 10m of a scooter and update scooter_users accordingly 
		update_scooter_users_a(user_name, lat, lon, scooters);
		
		try {
			const currScooter =  await client.db('VisiRide').collection('scooters').find({user : user_name}).sort({ 'rating' : -1 }).toArray();
			res.status(200).send(currScooter);
		} catch (err) {
			res.status(400).send('Invalid request!');
		}
		
	}catch(err){
		console.log(err);
        res.status(400).send('Invalid request!');
	}
	
});




var size = new Map();	
var oldKey = new Map(); 

app.post("/checkFace", urlParser, async (req, res)=>{
	try{
	
		console.log(req.body);
		// Check for the first request
		if(req.body.size !== undefined){

			// Set the file size for the image from the scooter to that scooter	
			size.set(req.body.scooterId, req.body.size);

			// initiate the array of old keys for that scooter
			oldKey.set(req.body.scooterId, []);
		}		


		// Iterate through every chunck in the request body
		let index = Object.keys(req.body).indexOf("scooterId");
		var currentScooter;
		if(!(typeof req.body[Object.keys(req.body)[index]] === 'string' || req.body[Object.keys(req.body)[index]] instanceof String)){
			currentScooter = req.body[Object.keys(req.body)[index]][0];
		} else {
			currentScooter = req.body[Object.keys(req.body)[index]];
		}
			

		for(var i = 0; i < Object.keys(req.body).length; i++) {
			var key = Object.keys(req.body)[i];

			if(key !== "size" && key !== "scooterId"){
				// get the current scooter Id
				
				// only check chunks that have not been added already
				if( !oldKey.get(currentScooter).includes(key) /*!oldKey.includes(key)*/) {
					//var appendData;
	
					// Make sure the chunk is just one string instead of an array of strings
					if(typeof req.body[key] === 'string' || req.body[key] instanceof String) {
						fs.appendFileSync('./scooterFaces64/' + currentScooter + ".txt", req.body[key]);
					}else {
						fs.appendFileSync('./scooterFaces64/' + currentScooter + ".txt" , req.body[key][0]);
					}
						
						// Add the chunk number to the oldkeys array for that specific scooter 
						let temp = oldKey.get(currentScooter);
						temp.push(key);
						oldKey.set(currentScooter, temp);


						// Check for the final chunk
						if(key === "data_" + size.get(currentScooter)){
							size.set(currentScooter, 0);
							console.log("final chunk!");
							oldKey.set(currentScooter, []);
		
							// Get the base 64 string that we got from the scooter
							//let s = fs.readFileSync('./scooterFaces64/' + currentScooter + ".txt", 'utf8' );
							
							fs.readFile('./scooterFaces64/' + currentScooter + ".txt", 'utf8', (err, data)=>{
								if(err) console.log(err);

								// Write the image using the base64 string 
								fs.writeFile("./scooterFacesJPG/" + currentScooter + "/checkFace.jpg", data, {encoding: 'base64'}, function(err){
									if(err)
										console.log(err);
									else{
										console.log("file created!!");

										fs.writeFileSync("./testImages" + "/checkFace" + currentScooter + ".jpg", data, {encoding: 'base64'});

			
										//clear the base64 contents of the text file	
										fs.writeFile('./scooterFaces64/' + currentScooter + ".txt", '', () =>{console.log('done');});
											

										// Upscale the image by 200%
										sharp("./scooterFacesJPG/" + currentScooter + "/checkFace.jpg")
										.rotate()
										.resize(720, 480) //200%
										.toFile("./scooterFacesJPG/" + currentScooter + "/checkFace_upscaled.jpg", (err) => {
											if(err) console.log(err);
											else{
												console.log("image upscaled");
			
												// delete the old image
												fs.unlink("./scooterFacesJPG/" + currentScooter + "/checkFace.jpg", async(err)=>{if(err) throw err;
													
													// run facial recognition 
													await checkFace("./scooterFacesJPG/" + currentScooter, currentScooter);
												});
											}
												
										});
									}
								});

							} );
							res.status(200).send("Final Chunk*");
						}
				} 
			}
		}
		if(size.get(currentScooter) !== 0)
			res.status(200).send("Chunk " + key + " recieved*");
	}catch(err){
		console.log(err)
		res.status(400).send("An error occured*");
	}
});


var faceRec_inProg = false;

app.post("/gps" , urlParser , async (req, res)=>{
	try{
		if(!(typeof req.body.scooterId === 'string' || req.body.scooterId instanceof String)) 
			var scooterId = req.body.scooterId[0];
		else 
			var scooterId = req.body.scooterId;

		const users = await client.db('VisiRide').collection('users').find().sort({ 'rating' : -1 }).toArray();

		if(req.body.valid === 'true'){	
			var lat = parseFloat(req.body.lat);
			var lon = parseFloat(req.body.lon);

			console.log("scooterId = " + scooterId);
			console.log("lat = " + lat);
			console.log("lan = " + lon);
			
			// update current location of scooter
			await client.db('VisiRide').collection('scooters').updateOne({ scooterId : scooterId }, 
				{ $set:{ location : { lat: lat, lon: lon } }});

			// get the scooter from the db to check if it is currently reserved or not
			const scooter = await client.db('VisiRide').collection('scooters').find({ scooterId : scooterId }).sort({ 'rating' : -1 }).toArray();

			// there is at least one user within 10 meters

			if(await update_scooter_users(scooterId, lat, lon, users) && !faceRec_inProg){
				if(scooter[0].user === "_none"){
					// lock and take a photo
					res.status(200).send("photo*"); 

				} else {
					// unlock 
					res.status(200).send("unlock*"); 
				}

			} else {
				// no user within 10 meters so lock
				res.status(200).send("lock*"); 
			}
		} else {			
			// get the scooter from the db to check if it is currently reserved or not
			const scooter = await client.db('VisiRide').collection('scooters').find({ scooterId : scooterId }).sort({ 'rating' : -1 }).toArray();

			if(await update_scooter_users(scooterId, scooter[0].location.lat, scooter[0].location.lon, users) && !faceRec_inProg){
				if(scooter[0].user === "_none"){
					// lock and take a photo
					res.status(200).send("photo*"); 
				}else {
					// unlock 
					res.status(200).send("unlock*"); 
				}
			} else {
				// no user within 10 meters so lock
				res.status(200).send("lock*"); 
			}
		}
	}catch(err){
		console.log(err);
		res.status(400).send("An error occured*");
	}
})


// needs testing 
app.post("/free_scooter", async (req,res) =>{
	try{
		let scooterId = req.body.scooterId;
		let username = req.body.username;

		let users = await client.db('VisiRide').collection('users').find({username : username}).sort({ 'rating' : -1 }).toArray();
		let time = users[0].time;

		await client.db('VisiRide').collection('scooters').updateOne({ scooterId : scooterId }, 
			{ $set: { user: "_none"} });

		await client.db('VisiRide').collection('users').updateOne({ username : username }, 
			{ $set: { time: 0} });

		let timeString = Date.now() - time;
 
		res.status(200).send(String(timeString));

	} catch(err) {
		console.log(err);
		res.status(400).send("An error occured");
	}
})











//////////////////////////////////////////////////////////////////////////////////////////////////////////////
app.listen(port,  () => {
	initiate_folders();
	initiate_scooter_users();
	console.log(`${new Date()}  App Started. Listening on ${host}:${port}`);
	
	

	
	//show_faces();
	//removeUsers();
	//removeScooters();
});
//////////////////////////////////////////////////////////////////////////////////////////////////////////////












function getSalt(length) {
    let result = '';
    const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    const charactersLength = characters.length;
    let counter = 0;
    while (counter < length) {
      result += characters.charAt(Math.floor(Math.random() * charactersLength));
      counter += 1;
    }
    return result;
}

function encrypt(password){

	if(typeof password !== "string")
		password = password.toString();

	var salt = getSalt(20);
	var hash = crypto.createHash('sha256')
	.update(password + salt)
	.digest('base64');
	return salt + hash;
};

var isCorrectPassword =function(password, saltedHash){
	var saltedPassword = password;
	var salt ='';
	for(let i = 0; i < 20; i++){
		salt += saltedHash[i];
	}
	saltedPassword += salt;
	let hash = crypto.createHash('sha256')
	.update(saltedPassword)
	.digest('base64');

	if(salt + hash === saltedHash)
		return true;
	else 
		return false;

	// 44 base64 of SHA256 of "salted password"
	// salted password = salt + password
};


function distance_meters(lat1, lon1, lat2, lon2) {
	var p = Math.PI / 180;   
	var c = Math.cos;
	var a = 0.5 - c((lat2 - lat1) * p)/2 + 
			c(lat1 * p) * c(lat2 * p) * 
			(1 - c((lon2 - lon1) * p))/2;	
  
	return 12742 * Math.asin(Math.sqrt(a)) * 1000; 
  }


async function update_scooter_users(scooterId, lat, lon, users){
	let scooter_user = await client.db('VisiRide').collection('scooters').find({scooterId : scooterId}).sort({ 'rating' : -1 }).toArray();

	let close_users = [];
	for(let i = 0; i < users.length; i++){
		if(distance_meters(users[i].deviceLocation.lat, users[i].deviceLocation.lon, lat, lon) <= 50 ){ 
			close_users.push(users[i].username);
		} else {
			console.log("!!!dist!!!: " + distance_meters(users[i].deviceLocation.lat, users[i].deviceLocation.lon, lat, lon));
		}
	}
	scooter_users.set(scooterId,close_users);
	console.log(scooter_users);
	console.log(close_users.length + " ** " + scooter_user[0].user);
	return (close_users.length !== 0);
			
}

function update_scooter_users_a(username, lat, lon, scooters){
	
	for(let i = 0; i < scooters.length; i++){
		if(distance_meters(scooters[i].lat, scooters[i].lon, lat, lon) <= 50 && 
			!scooter_users.get(scooters[i].scooterId).includes(username)){
				let temp = scooter_users.get(scooters[i].scooterId);
				temp.push(username);
				scooter_users.set(scooters[i].scooterId, temp);	
				
		}else if(distance_meters(scooters[i].lat, scooters[i].lon, lat, lon) > 50 &&
				scooter_users.get(scooters[i].scooterId).includes(username)){
					let temp = scooter_users.get(scooters[i].scooterId);
					temp.splice(temp.indexOf(username),1);
					scooter_users.set(scooters[i].scooterId, temp);	
				}
	}
	console.log(scooter_users);
	
}


async function checkFace(face_to_check, scooterId){
	let close_users = scooter_users.get(scooterId);
	console.log(close_users);
	const folder_name = "./scooterCloseUsers/" + scooterId;

/*
	fs.mkdirSync(path.join(__dirname,  folder_name), (err) => {
		if (err) return console.error(err);
	});
*/
	

	try{
		const users =  await client.db('VisiRide').collection('users').find().sort({ 'rating' : -1 }).toArray();

		for(let i = 0; i < users.length; i++){
			if(close_users.includes(users[i].username)){
				fs.writeFile("./" + folder_name +"/ " +  users[i].username + ".jpg", users[i].image, {encoding: 'base64'}, function(err){
					if(err)
						console.log(err);
					else
						console.log("file created!!");
				});

			}
		}

		faceRec_inProg = true;

		exec("face_recognition " + "./" + folder_name + " ./" + face_to_check, async (err, stdout, stderr) =>{
			if(err) console.log(stderr);

			stdout = stdout.slice(0, stdout.length - 1);
			var face_index = stdout.split(",").length - 1;
			var face_name = stdout.split(",")[face_index];

			// var face_name = face_name.substring(0, face_name.length - 1);
			
			// delete all images of close users
			fs.readdir(folder_name, (err, files) => {
				if (err) throw err;
				for (const file of files) {
				  fs.unlink(path.join(folder_name, file), (err) => {
					if (err) throw err;
				  });
				}
			});

			faceRec_inProg = false;
			
			if("unknown_person" !== face_name && "no_persons_found" !== face_name){
				face_name = face_name.substring(1, face_name.length);

				// Update the user 
				await client.db('VisiRide').collection('scooters').updateOne({ scooterId : scooterId }, 
					{ $set: { user: face_name} });

				await client.db('VisiRide').collection('users').updateOne({ username : face_name }, 
					{ $set: { time: Date.now()} });

				console.log("-------FACE RECOGNIZED, Welcome " + face_name);
				// delete the image from the scooter
				fs.unlink("./scooterFacesJPG/" + scooterId + "/checkFace_upscaled.jpg",(err)=>{
					if(err) throw err;
				});

				return true;
			}else{
				console.log("-------FACE NOT RECOGNIZED");
				// delete the image from the scooter
				fs.unlink("./scooterFacesJPG/" + scooterId + "/checkFace_upscaled.jpg",(err)=>{
					if(err) throw err;
				});
				return false;
			}		
        });
	} catch(err){

		throw(err);
	}
};

function isValid(username){
	if (/\s/.test(username) || username.length > 30 || username.length < 6 || username.includes(".") || username === "unknown_person" || username === "no_persons_found"){
		console.log("invalid");
		return false;
	}
		
	return true;
	
}

async function isTaken(username){
	const users = await client.db('VisiRide').collection('users').find({username : username}).sort({ 'rating' : -1 }).toArray();
	return (users.length === 1);
}


// for testing purposes only
async function show_faces(){
	try{
		const users = await client.db('VisiRide').collection('users').find().sort({ 'rating' : -1 }).toArray();
		for(let i = 0; i < users.length; i++){
			fs.writeFile("./show_face/" + users[i].username + ".jpg", users[i].image, {encoding: 'base64'}, function(err){
				if(err)
					console.log(err);
				else
					console.log("file created!!");
			});
		}
	}catch(err){
		console.log(err);
	}

}




//testing
	/*
	scooter_users.set("scooter1" , ["joe_biden","messi22335"]);
	checkFace("temp_faces/biden0.jpg", "scooter1");
	//show_faces();
	//
	*/

async function removeUsers(){
	try{
		await client.db('VisiRide').collection('users').drop();
		console.log("removed users");
	}catch(err){
		console.log(err);
	}
	
}


async function removeScooters(){
	try{
		await client.db('VisiRide').collection('scooters').drop();
		console.log("removed scooters");
	}catch(err){
		console.log(err);
	}
	
}


async function initiate_scooter_users(){
	const scooters =  await client.db('VisiRide').collection('scooters').find().sort({ 'rating' : -1 }).toArray();
	const users =  await client.db('VisiRide').collection('users').find().sort({ 'rating' : -1 }).toArray();
	
	for(let i = 0; i < scooters.length; i++){
		let close_users = [];
		for(let t = 0; t < users.length; t++){
			if(distance_meters(users[t].deviceLocation.lat, users[t].deviceLocation.lon, scooters[i].location.lat, scooters[i].location.lon) <= 10){
				close_users.push(users[t].username);
			}
		}
		scooter_users.set(scooters[i].scooterId,close_users);
	}

	console.log(scooter_users);
}

function upscaleFaces(img){
	sharp('./de1_faces_unedited/' + img + ".jpg")
	.rotate()
	.resize(720, 480)
	.toFile('./de1_faces_edited/' + img + "_upscaled.jpg", function(err) {
		if(err) console.log(err);
		else
			console.log("image upscaled");
	});
}
function sharpenFace(img){
	sharp('./de1_faces_edited/' + img + "_upscaled.jpg")
	.sharpen({ sigma: 4 })
	.toFile('./de1_faces_edited/' + img + "_sharpened.jpg", function(err) {
		if(err) console.log(err);
		else
			console.log("image sharpened");
	});
}

function initiate_folders(){
	let folders = ["./scooterFaces64", "./scooterFacesJPG", "./scooterCloseUsers"];
	for (let folder_name of folders){
		if (!fs.existsSync(folder_name)) {
			fs.mkdirSync(folder_name, (err) => {
				if (err) 
					console.error(err);

				
			});
		}
	}
	
}

function to_hhmmss(time){
	const sec = Math.floor((time / 1000) % 60);
	const min = Math.floor((time / 1000 / 60) % 60);
	const hour = Math.floor((time  / 1000 / 3600 ) )

	return hour + ":" + min + ":" + sec;
}