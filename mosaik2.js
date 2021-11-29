// mosaik2

/*
 *
 */

//runs with nodejs v14.15.4

let fs = require('fs');
let fs_promises = fs.promises;
if(!fs_promises)
	fs_promises = require("fs").promises;



//require("fs/promises");
if(!fs_promises )
{
	console.error("fs.promises could not be loaded");
process.exit(1);
}
//let fs_promises = require('fs/promises');
let https = require("https");
const { finished } = require("stream"); //register broken http downloads during download
let child_process = require("child_process");
const coreNumber = require('os').cpus().length;
const DB_VERSION = 4;

var args = process.argv.slice(2);
let debug=false;
let debugv=false;
process.title="mosaik2";

let CTX;

function Mosaik2Job(ctx) {

	this.ctx = ctx;

	this.filesize;
	this.imagedims;
	this.tiledims;
	this.pixelPerTile;
	//this.colorCount;
	this.colors;
	this.stddev;
	this.filename;
	this.filehash;
	this.timestamp;
	this.count =ctx.count++;
	this.replacer = new RegExp("_", 'g')
	this.state = this.INITIAL_STATE;

	// measures only the file loading time, making it possible to prevent connection resets
	this.loadStart = undefined;
	this.loadEnd = undefined;
	this.stream;

	this.filetypeJpegPattern = /\.jpe?g$/i;
	this.filetypePngPattern = /\.png$/i;
	this.filetype;
	this.filesize;

	this.data; //byte buffer of image

	this.INITIAL_STATE = 0;
	this.LOADING_STATE = 1;
	this.INDEXING_STATE = 2;
	this.WRITING_INDEX_STATE = 3;
	this.ENDING_STATE = 4;

	this.exec = async function(data) {
		try {

			data = data.split("\t");
			this.resource = data[0];
			this.filesize = parseInt(data[1]);
			this.timestamp = parseInt(data[2]);

			let name = this.resource.split("/");
			name = decodeURIComponent(    name[ name.length-1 ]    ).replace(this.replacer," ");

			if(this.filetypeJpegPattern.test(this.resource)) {
				this.filetype="jpeg";
			//} else if(this.filetypePngPattern.test(this.resource)) {
			//	this.filetype="png";
			} else {
				throw "#"+this.count+" unsupported file type (prematched by file extension):"+name;
			}


			let t0 = Date.now();
			let stream = await this.loadResource(this,this.resource);

			//const isUnique = await this.isResourceUnique(this.ctx,this.filehash);
			//if(isUnique) {
			const indexData = await this.indexResource(this.ctx,this.data,this.filehash,this.resource,name,this,stream,this.timestamp);
			//call synced
			this.writeIndexToFile( this.ctx, indexData );

			let idx = this.ctx.runningTasks.indexOf(this);
			this.ctx.runningTasks.splice( idx, 1);
			
			if(debugv)console.log(" ".repeat(ctx.runningTasks.length)+"-> finished  job #"+this.count+", jobs: "+ctx.runningTasks.length, /*ids,*/ " img/s:"+Math.round(	(ctx.count-this.ctx.initialCount)/ ((new Date()-this.ctx.t0)/1000),2),"px/ms:"+Math.round(this.ctx.pixel/((new Date()-this.ctx.t0))), "bytes/ms:"+ Math.round( this.ctx.bytes/((new Date()-this.ctx.t0)  ) ));
			if(this.count%1==0)console.log(new Date()+" finished  job #"+this.count+", jobs: "+ctx.runningTasks.length, /*ids,*/ " img/min:"+Math.round(	(ctx.count-this.ctx.initialCount)/ ((new Date()-this.ctx.t0)/60000),2),"px/ms:"+Math.round(this.ctx.pixel/((new Date()-this.ctx.t0))), "bytes/ms:"+ Math.round( this.ctx.bytes/((new Date()-this.ctx.t0)  ) )+", loadavg:"+this.ctx.loadavg);
			//console.log("running tasks 3",this.ctx.runningTasks.length,"age:",new Date() - age);
		} catch (e) {
			//console.error();
			let idx = this.ctx.runningTasks.indexOf(this);
			this.ctx.runningTasks.splice( idx, 1 );
			if(debug||debugv)console.error("EXEC FAILED   #"+this.count+", jobs: "+ctx.runningTasks.length, e);
			if(debugv)console.log(" ".repeat(ctx.runningTasks.length)+"finished  job #"+this.count+", jobs: "+ctx.runningTasks.length, /*ids,*/ " img/s:"+Math.round(	(ctx.count-this.ctx.initialCount)/ ((new Date()-this.ctx.t0)/1000),2),"px/ms:"+Math.round(this.ctx.pixel/((new Date()-this.ctx.t0))), "bytes/ms:"+ Math.round( this.ctx.bytes/((new Date()-this.ctx.t0)  ) ));
		}
		/*if(this.ctx.runningTasks.length<10 && process.stdin.isPaused() == true) {
			if(debug) {
				console.log("runningTasks",this.ctx.runningTasks.length);
				console.log("less than 10 running tasks, resuming stdin"	);
			}
			process.stdin.resume();
		}*/
	}

	this.loadResource = function(thiz,resource) {
		return new Promise( (resolve, reject) => {

			if(resource.startsWith("https://")||resource.startsWith("http://")) {
				thiz.state = thiz.LOADING_STATE;
				thiz.loadStart = Date.now();
				//TODO hard coded hostname, that is no good
				let opt = {method:"GET", hostname: "upload.wikimedia.org", port:443, path: resource.substr(28), agent: ctx.agent}; //should use keep alive
				let req = https.request(opt, function(resp) {
					thiz.filesize = resp.headers["Content-Length"]||resp.headers["content-length"];
					if(!thiz.filesize) {
						req.end();
						this.loadEnd = Date.now();
						reject("unknown filesize, content-length header is invalid");
					}
					thiz.filesize = parseInt(thiz.filesize);
					if(resp.statusCode==200)
						resolve(resp);
					else {
						req.end();
						thiz.loadEnd = Date.now();
						reject("invalid http response with code:"+resp.statusCode);
					}
				});
				req.on("error",function(e) {
					req.end();
					thiz.loadEnd = Date.now();
					console.error("Error on http execute",e);
					reject(e);
					return;
				});
				req.end();

			} else {
				thiz.state = thiz.LOADING_STATE;
				thiz.loadStart = new Date();
				resolve(fs.createReadStream(resource));
			}
		});
	}
	this.isResourceUnique = function(ctx,hash) {
		return new Promise( (resolve, reject) => {
			if(ctx.initialCount==0) {
				resolve(true);
				return;
			}
			let opts = {emitClose:true,autoClose:true};
			let stream = fs.createReadStream(ctx.thumbsDbFiles.filehashes);
			stream.on("data", function(chunk) {
				if(chunk.length%16!=0) { console.error("chunk is no multple of 16, fatal exit"); process.exit(16); }
				for(var i = 0,l=chunk.length;i<l;i+=16) {
					if( hash[0 ]==chunk[i+0 ] && hash[1 ]==chunk[i+1 ] && hash[2 ]==chunk[i+2 ] && hash[3 ]==chunk[i+3 ] &&
					    hash[4 ]==chunk[i+4 ] && hash[5 ]==chunk[i+5 ] && hash[6 ]==chunk[i+6 ] && hash[7 ]==chunk[i+7 ] &&
					    hash[8 ]==chunk[i+8 ] && hash[9 ]==chunk[i+9 ] && hash[10]==chunk[i+10] && hash[11]==chunk[i+11] &&
					    hash[12]==chunk[i+12] && hash[13]==chunk[i+13] && hash[14]==chunk[i+14] && hash[15]==chunk[i+15] ) {
						resolve(false); // hash already exists, so it is not unique
						stream.close();
						return;
					}
				}
			stream.on("close", ()=>{resolve(true)});
			stream.on("error", (err)=>{console.error("hash error event",err);reject(err);return;});
			});
		});
	}


	//sync function, to ensure to write only one at a time
	this.writeIndexToFile = function(ctx,indexData) {
		this.state = this.WRITING_INDEX_STATE;
		//console.log(this.count, "write index start", new Date());
		let keys = Object.keys( indexData );

		//old
	/*	console.log(new Date(), "wait for all ...");

		keys.forEach( key => {
			let path = this.ctx.thumbsDbFiles[ key ];
			console.log(new Date(), "starte appendFileSync", path);

			fs.appendFileSync( path, indexData[key] );
		});
		console.log(new Date(), "wait for all finished");
*/
		let promises = [];
		keys.forEach( key => {
			let path = ctx.thumbsDbName+'/'+ctx.thumbsDbFiles[key];
			if(debug)console.log(new Date(), "starte appendFile", path);
			promises.push( fs_promises.appendFile(path, indexData[key] ) );
		});
		//console.log(new Date(), "wait for all ...");
		Promise.all(promises);
		//console.log(new Date(), "wait for all finished");


		this.state = this.ENDING_STATE;
		//console.log(this.count, "write index end", new Date());
	}


	this.indexResource = function(ctx,data,filehash,filename,name,thiz,stream,timestamp) {
		return new Promise( (resolve, reject) => {
			thiz.state = thiz.INDEXING_STATE;

			let command = "./tiler_hex_md5 "+ ctx.tileEdgeCount+ " " + thiz.filesize;
			let stdout0 = undefined;
			let stderr0 = undefined;
			if(debug)console.error("calling ", command);
				let child = child_process.exec(command, { timeout: 5*60*1000 }, (error, stdout, stderr) => {
				if (error) {
//				    console.error(!`error: ${error.message}`);
				}
				if (stderr) {
					stderr0=stderr;
				    //console.error(`stderr: ${stderr} @ ${filename}`);
				}
//				console.log(`stdout: ${stdout}`);
				stdout0 = stdout;
			});
			child.on("close", function(code) {
				if(code==0) {
					//examine stdout

					
					let filesizesBuffer = Buffer.alloc(4);
					filesizesBuffer.writeUInt32LE( thiz.filesize );

					let stdoutSplit = stdout0.split(" ");
					let width = parseInt(stdoutSplit[0], 16);
					let height = parseInt(stdoutSplit[1], 16);
					let tileXCount = parseInt(stdoutSplit[2], 16);
					let tileYCount = parseInt(stdoutSplit[3], 16);

					let imageDimBuffer = Buffer.alloc(8);
					imageDimBuffer.writeUInt8((width >>24)&255 , 0);
					imageDimBuffer.writeUInt8((width >>16)&255 , 1);
					imageDimBuffer.writeUInt8((width >>8) &255 , 2);
					imageDimBuffer.writeUInt8((width      &255), 3);
					imageDimBuffer.writeUInt8((height>>24)&255 , 4);
					imageDimBuffer.writeUInt8((height>>16)&255 , 5);
					imageDimBuffer.writeUInt8((height>>8) &255 , 6);
					imageDimBuffer.writeUInt8((height     &255), 7);

					let tileDimBuffer = Buffer.alloc(2);
					tileDimBuffer.writeUInt8(tileXCount,0);
					tileDimBuffer.writeUInt8(tileYCount,1);

					let totalTileCount = tileXCount*tileYCount;
					let colorsBufferLen = totalTileCount*3;
				  let colorsBuffer = Buffer.alloc(colorsBufferLen);//+2 for x and y tile cout and per tile 3 byte colorinfos
					let stddevBuffer = Buffer.alloc(colorsBufferLen); // *3 for rgb

					let b1,b2,j=0,k=0,len=stdoutSplit.length-1; //last is md5 hash
					for(let i=4;i<len;i+=2) {

						b1 = parseInt( stdoutSplit[i],16 );
						b2 = parseInt( stdoutSplit[i+1],16 );

						colorsBuffer.writeUInt8((b1 >>16)&255 , j++);
						colorsBuffer.writeUInt8((b1 >>8) &255 , j++);
						colorsBuffer.writeUInt8((b1      &255), j++);
						stddevBuffer.writeUInt8((b2>>16) &255 , k++);
						stddevBuffer.writeUInt8((b2>>8)  &255 , k++);
						stddevBuffer.writeUInt8((b2      &255), k++);
					}

					let hash = stdoutSplit[ stdoutSplit.length-1 ];

					let hashBuffer = Buffer.alloc(16);
					hashBuffer.writeUInt8( parseInt( hash.substr(0 ,2), 16 ),0 );
					hashBuffer.writeUInt8( parseInt( hash.substr(2 ,2), 16 ),1 );
					hashBuffer.writeUInt8( parseInt( hash.substr(4 ,2), 16 ),2 );
					hashBuffer.writeUInt8( parseInt( hash.substr(6 ,2), 16 ),3 );
					hashBuffer.writeUInt8( parseInt( hash.substr(8 ,2), 16 ),4 );
					hashBuffer.writeUInt8( parseInt( hash.substr(10,2), 16 ),5 );
					hashBuffer.writeUInt8( parseInt( hash.substr(12,2), 16 ),6 );
					hashBuffer.writeUInt8( parseInt( hash.substr(14,2), 16 ),7 );
					hashBuffer.writeUInt8( parseInt( hash.substr(16,2), 16 ),8 );
					hashBuffer.writeUInt8( parseInt( hash.substr(18,2), 16 ),9 );
					hashBuffer.writeUInt8( parseInt( hash.substr(20,2), 16 ),10 );
					hashBuffer.writeUInt8( parseInt( hash.substr(22,2), 16 ),11 );
					hashBuffer.writeUInt8( parseInt( hash.substr(24,2), 16 ),12 );
					hashBuffer.writeUInt8( parseInt( hash.substr(26,2), 16 ),13 );
					hashBuffer.writeUInt8( parseInt( hash.substr(28,2), 16 ),14 );
					hashBuffer.writeUInt8( parseInt( hash.substr(30,2), 16 ),15 );
					
					let timestampsBuffer = Buffer.alloc(8);
					try {
					timestampsBuffer.writeUInt32LE( timestamp ); // TODO check binary format
					} catch( e ) {
						console.error( filename );
						console.error( e );
					}


					let filesizeIndex = 
						fs.statSync(  ctx.thumbsDbName +"/"+ctx.thumbsDbFiles.filenames  ,{bigint:true}).size;
					let filenamesIndexBuffer = Buffer.alloc(8);
					filenamesIndexBuffer.writeBigUInt64LE(
						filesizeIndex
					);

					let invalidBuffer = Buffer.alloc(1);
					invalidBuffer.writeUInt8( 0, 0 );

					let duplicatesBuffer = Buffer.alloc(1);
					duplicatesBuffer.writeUInt8( 0, 0 );


					var indexData = {
						"filesizes": filesizesBuffer,
						"filehashes": hashBuffer,
						"timestamps": timestampsBuffer,
						"filenames": filename+"\n",
						"filenamesindex": filenamesIndexBuffer,
						"imagedims": imageDimBuffer,
						"tiledims": tileDimBuffer,
						"imagecolors": colorsBuffer,
						"imagestddev": stddevBuffer,
						"invalid": invalidBuffer,
						"duplicates": duplicatesBuffer
					};

					thiz.ctx.pixel += width * height;
					thiz.ctx.bytes += thiz.filesize;

					resolve(indexData);

				} else {

					// bad exit code of tiler_hex sub command
					thiz.loadEnd = Date.now();
					reject("file could not be tiled by subprogram tiler_hex, exit-code:"+code+", message:"+stderr0);

				}
			});
			stream.pipe( child.stdin );
			child.stdin.resume();
			stream.on('end', () => {
				if(debug) console.log("####### stream to tiler_hex has ended");
				thiz.loadEnd = Date.now();
			});
			/*finished(stream, (err) => {
				if(err) {
					console.error("finished stream failed.", err);
					try {
						console.error("unpiped childs input");
						stream.unpipe( child.stdin );
						console.error("unpiped!");
					} catch (e) {
						console.error("error while unpiping", e);
					}
				}

			});*/
		});
	}

}

function initCtx(thumbsDbName, tileEdgeCount) {

/*ls ../../mosaik2/mosaik2_test_database_2003
conf.json       filenames.txt  imagecolors.bin  imagestddev.bin  tilecount.conf
filehashes.bin  filesizes.bin  imagesdims.bin   invalid.bin      tiledims.bin
*/

	var ctx = {};
	if(!tileEdgeCount) {
		tileEdgeCount = JSON.parse(fs.readFileSync(thumbsDbName+"/tilecount.txt"));
		}
	
	ctx.tileEdgeCount = tileEdgeCount;
	ctx.thumbsDbName = thumbsDbName; //name of directory
//	ctx.thumbsDbConf = thumbsDbName+'.conf.json';
	ctx.thumbsDbFiles = {
		 "filesizes": 'filesizes.bin',
		 "filenames": 'filenames.txt',
		 "filenamesindex": 'filenames.idx',
		 "filehashes": 'filehashes.bin',
     "timestamps": "timestamps.bin",
		 "imagedims": 'imagedims.bin',
		 "tiledims": 'tiledims.bin',
		 "imagecolors": 'imagecolors.bin',
		 "imagestddev": 'imagestddev.bin',
		 "version": 'dbversion.txt',
		 'invalid': 'invalid.bin',
     "duplicates": "duplicates.bin",
		 'tilecount': 'tilecount.txt'
		// "imagepixelpertile": thumbsDbName+'.db.imagepixelpertile',
		// "imagecolorcount":  thumbsDbName+'.db.imagecolorcount',
		// "imagetileedgecount": thumbsDbName+'.db.imagetileedgecount'
	};
	ctx.filehashAlreadyIndexedTime = 0;
	ctx.filehashAlreadyIndexedCount = 0;
	ctx.configNameRegexp = /^[a-zA-Z0-9]{1,100}$/
	return ctx;
}

function loadCtx(thumbsDbName) {

	//var configFile = './'+thumbsDbName+'.conf.json';
	//if (!fs.existsSync(configFile)) {
	//	console.error("loadCtx: conf-file does not exists, exit ");
//		process.exit(1);
//	}


	let ctx = initCtx(thumbsDbName, undefined);//JSON.parse(fs.readFileSync(configFile));
	CTX = ctx;
	//check how many files have been processed in past calls
	//it is for fs.createReadStream, because it does not release end event on empty files
	ctx.initialCount = fs.statSync(ctx.thumbsDbName+'/'+ctx.thumbsDbFiles.filehashes).size/16;
	ctx.count = ctx.initialCount;
	ctx.pixel = 0;
	ctx.loadavg = 0.0;
	ctx.bytes = 0;
	ctx.t0 = new Date();
	ctx.exiting = false;
	ctx.runningTasks = [];
	ctx.mode = args[0];
	ctx.pidFile = thumbsDbName+".pid";

	var dbVersionFile = thumbsDbName+'/'+ctx.thumbsDbFiles.version;
	if(!fs.existsSync(dbVersionFile)) {
		console.error("loadCtx: mosaik2 db version file does not exists, exit");
		process.exit(1);
	}

	let dbVersion = JSON.parse(fs.readFileSync(dbVersionFile));
	if( dbVersion != DB_VERSION ) {
		console.error("loadCtx: db.version ("+dbVersion+") does not match with the expected one ("+DB_VERSION+"), exit");
		process.exit(1);
	}
	ctx.dbVersion = dbVersion;

	return ctx;
}

function printVersion() {
	console.log("mosaik2 v0.1 - photo mosaic program for huge data sets");
}

function printUsage() {
	console.log("usage:\nnodejs mosaik2.js { init | clean | index } mosaik2_db_dir [ options ]\n_moasik2_db_dir: is a text with the valid signs 0-9, a-z and A-Z, maximum length is 100\noptions init: edge_count_tiles # ( number ) smaller dimension of source image will split in analyzing process at least into this amount of square tiles on the smaller side, the longer side will may have more tiles\noptions index: max_tiler_processes max_loadavg # ( both params are grouped, they are optional; max_tiler_processes => integer, min 1, default cpu cores * 8; max_loadavg => float, min 1.0, default cpu cores * 2.0 - 1");
}









if(args[0] == "kill") {


	try {
		let ctx = loadCtx( args[1] );
		let pid = JSON.parse( fs.readFileSync(ctx.thumbsDbName+'/'+ctx.pidFile, {encoding:"utf8"}) );
		console.log(pid, typeof pid);
		process.kill( pid, 'SIGTERM');
	} catch( e ) {
		console.error("unable to detect pid to SIGTERM, nothing killed", e);
	}
	process.exit(0);

} else if(args[0] == "clean") {

	if(args.length!=2) {
		console.error("wrong parameter");
		printUsage();
		process.exit(1);
	}

	var ctx = loadCtx(args[1]);

	if (fs.existsSync(ctx.thumbsDbConf)) {
		console.log("deleting",ctx.thumbsDbConf);
		fs.unlink(ctx.thumbsDbConf, function (err,data) { if(err) {console.error(err);process.exit(1);} });
	}

	var thumbsDbFiles = Object.values(ctx.thumbsDbFiles);
	for(var i=0;i<thumbsDbFiles.length;i++) {

		if (fs.existsSync(thumbsDbFiles[i])) {
			console.log("deleting",thumbsDbFiles[i]);
			fs.unlink(thumbsDbFiles[i], function (err,data) { if(err) {console.error("error deleting file",err);process.exit(1);} });
		} else {
			console.error(thumbsDbFiles[i],"NOT deleted, does not exist");
		}
	}


} else if(args[0] == "init") {

	if(args.length!=3) {
		console.error("wrong parameter");
		printUsage();
		process.exit(1);
	}

	var ctx = initCtx(args[1],args[2]);
	if(! ctx.configNameRegexp.test(args[1]) || isNaN(args[2]) ) {
		console.error("wrong parameter values, exit" );
		printUsage();
		process.exit(1);
	}

	if (fs.existsSync( ctx.thumbsDbName )) {
		console.error("mosaik2 database directory already exists, exit");
		process.exit(1);
	}

	fs.mkdirSync( ctx.thumbsDbName );
	console.log( "mosaik2 database directory created" );

	ctx.initialCount = 0;

	//fs.writeFile(ctx.thumbsDbConf, JSON.stringify(ctx), function(err) {if(err) {console.error("error writing file",err);process.exit(1);}});

	var thumbsDbFiles = Object.values(ctx.thumbsDbFiles);
	for(var i=0;i<thumbsDbFiles.length;i++) {
		console.log('creating db file', thumbsDbFiles[i]	);
		fs.writeFile(ctx.thumbsDbName+'/'+thumbsDbFiles[i], '', function(err) {if(err) {console.error(err);process.exit(1);}});
	}

	let readonlyFiles = [ctx.thumbsDbFiles.tilecount, ctx.thumbsDbFiles.version];
	for(let i=0;i<readonlyFiles.length;i++) {
//		fs.chmod(readonlyFiles[i], 444
	}

	fs.writeFile(
		ctx.thumbsDbName+'/'+ctx.thumbsDbFiles.tilecount, ""+ctx.tileEdgeCount, {flag: 'a'}, function(err) {
			if(err){
				console.error("error writing file", err); process.exit(1);
			}
			console.log("saved tile count");
		}
	);
	fs.chmod(ctx.thumbsDbName+'/'+ctx.thumbsDbFiles.tilecount,0o444,(err)=>{
		if(err) {
			console.error("error while set readony the tile count file");
		}
	});
	fs.writeFile(
		ctx.thumbsDbName+'/'+ctx.thumbsDbFiles.version, ""+DB_VERSION, {flag: 'a'}, function(err){
			if(err){
				console.error("error writing file", err); process.exit(1)
			}
			console.log("saved db version");
		}
	);
	fs.chmod(ctx.thumbsDbName+'/'+ctx.thumbsDbFiles.version,0o444,(err)=>{
		if(err) {
			console.error("error while set readonly the db version file");
		}
	});

} else if( args[0] == "index" ) {
	
	index(args);

} else {
	if(!args[0]) {
		console.error("wrong parameter");
		printUsage();
		process.exit(1);
	}
	process.exit(0);
}

var c=0;
async function index(args) {

	if(debug) console.error("processing index function");



	if(!(args.length == 2 || args.length == 4)) {
		console.error("wrong parameter");
		printUsage();
		process.exit(1);
	}

	let max_tiler_processes;
  let max_loadavg;
  if(args.length == 4) {
    max_tiler_processes = parseInt(args[2]);
    if(max_tiler_processes < 1) {
      console.error("too low max_tiler_processes, should be above 0");
      process.exit(1);
    }
    max_loadavg = parseFloat( args[3] );
    if( max_loadavg < 1.0) {
      console.error("too low max_loadavg, should be above 1.0");
      process.exit(1);
    }
  } else {
    max_tiler_processes = coreNumber * 5; // bei 2 cores am besten
    max_loadavg = coreNumber * 5;
  }

  console.log("max_tiler_processes", max_tiler_processes, "max_loadavg", max_loadavg);



	var ctx = loadCtx(args[1]);
	ctx.max_tiler_processes = max_tiler_processes;

	ctx.agent = new https.Agent({ keepAlive: true });



	if (fs.existsSync(ctx.pidFile)) {
		console.error("pid-file "+ctx.pidFile+" exist, mosaik2 seems to be running");
		process.exit(1);
	}
	fs.writeFileSync(ctx.pidFile, JSON.stringify(process.pid), {encoding:"utf8"});

	var linecounter = 0;


	process.stdin.resume();
	process.stdin.setEncoding('utf8');
	var lingeringLine = "";


	let c=0; // counter for stdin data calls below
	process.stdin.on('data', function(chunk) {

		process.stdin.pause();
		let lines = chunk.split("\n");
		lines[0] = lingeringLine + lines[0];
		lingeringLine = lines.pop();
		//linecounter++;
		if(debugv)console.log(" *** stdin data received #"+c+", lines:"+lines.length+ " stdin now PAUSED");
		if(debug)console.log(" *** "+chunk);
		executeNextLine(lines,0,c++);
	});

//TODO wird ggf häufiger aufgerufen sodass zuviele Abfragen stattfinden
	function executeNextLine( lines, i, c ) {

		if(debug)console.log("process stdin lines, c:"+ c+ " progress => idx:"+ (i) +" from len:"+lines.length+ " "+lines[i]	);

		if( i >= lines.length ) {
			if(debug) console.log(" +++ all input resource locations are processed, resuming stdin");
			if(ctx.exiting == true) {
				console.log("stdin is not resumed, EXITing because of SIGTERM.");
			} else {
				process.stdin.resume();
			}
			return;
		}

		let now = Date.now();
		let longestLoadTime = 0;
		for(let j=0; j<ctx.runningTasks.length; j++) {
			let loadStart = ctx.runningTasks[j].loadStart;
			let loadEnd = ctx.runningTasks[j].loadEnd;
			let rightState = ctx.runningTasks[j].state == ctx.runningTasks[j].LOADING_STATE;
			// only look at pending downloads, which have no loadEnd timestamp yet
			if(rightState ==true && loadStart && !loadEnd) {
				if(now-loadStart> longestLoadTime) {
					longestLoadTime = now-loadStart;
					longestLoadTimeSize = ctx.runningTasks[j].filesize;
				}
			}
		}

		let loadavg = fs.readFileSync("/proc/loadavg",  {encoding:'utf8', flag:'r'});
		ctx.loadavg = parseFloat(loadavg.split(" ")[0]);

		let maxJobs = ctx.max_tiler_processes;
		let minJobs = 1; //no matter, one job should run always at a time
		let longestAllowedLoadTime = 5000;
		let loadavgAllowed = max_loadavg;
		let waitTimeInMs = 100;
		let waitTimeInMsNextLine = 10;

		if( ctx.runningTasks.length < minJobs
			|| ctx.runningTasks.length < maxJobs
			&& longestLoadTime < longestAllowedLoadTime
			&& ctx.loadavg < loadavgAllowed) {

			let x = new Mosaik2Job(ctx);

			ctx.runningTasks.push(x);


			if(debugv)console.log(" ".repeat(ctx.runningTasks.length)+"start new job #"+x.count+ ", jobs: "+ ctx.runningTasks.length/*, */);

			x.exec(lines[i] );
			if(i+1<lines.length) {
				setTimeout(executeNextLine,waitTimeInMsNextLine,lines,i+1,c);
			} else {

				if(ctx.exiting== false) {
					if(debugv) console.log(" +++ all input resource locations are processed, resuming stdin");
					process.stdin.resume();
				} else {
					console.log(" stdin stays paused, soft exit");

				}
			}
			//executeNextLine(lines,i+1,c);
		} else {
			if(debug) {
				//console.log(  "** verzögere neue Jobs um "+waitTimeInMs+" ms");
				let localWaitTimeInMs = waitTimeInMs;
				if(ctx.runningTasks.length >= maxJobs) {
					console.log("   too much running jobs:"+ctx.runningTasks.length);
				}
				if(longestLoadTime >= longestAllowedLoadTime ) {
					console.log("   there is download pending too long, waiting until thats finished longest:" + longestLoadTime + ", downloadsize:"+longestLoadTimeSize);
				 }
				if( loadavg >= loadavgAllowed ) {
					console.log("   there is too much load on this machine:"+loadavg);
				}
			 }
			//process.stdout.write('.')
			setTimeout(executeNextLine,waitTimeInMs,lines,i,c);
		}
		//if(debug)console.log("exiting recfn        c:"+ c+ " progress => idx:"+ i+   " from len:"+lines.length+" " + lines[i]);
		//TODO and oldes Tasks not too old
	}
}



process.on('exit', function (){
	if(debug)console.log("on exit");
//	 fs.writeFile(ctx.thumbsDbConf, JSON.stringify(ctx), function(err) {if(err) {console.log(err);process.exit(1);}});
	deletePidFile(CTX);
});

process.on('SIGTERM', function() {
	CTX.exiting=true;
	console.log("SIGTERM received, intiating soft exit");
});

process.on('SIGUSR1', function() {
	CTX.max_tiler_processes += 1;
	console.log("SIGUSR1 receivid, increase parallel job count to", CTX.max_tiler_processes); 
});

process.on('SIGUSR2', function() {
	CTX.max_tiler_processes -= 1;
	console.log("SIGUSR2 receivid, decrease parallel job count to", CTX.max_tiler_processes); 
});

process.on('uncaughtException', function(err) {

	  console.error("mosaik2 uncaughtException",err);

    // Handle the error safely
    //console.error('uncaughtException',err)
});

function deletePidFile(ctx) {
	if( ctx && ctx.mode == "index" ) {
		try {
			fs.unlinkSync(ctx.pidFile);
		} catch ( e ) {
			console.error( "unable to delete "+ctx.pidFile );
		}
	}
}
