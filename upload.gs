var baseFolder = "Broadcast 408/Timelapses/" // make sure to add last "/" (e.g. "parent/child/")

function doPost(e) {
  
  // grab parameters
  var folderName = e.parameter.folderName; // placed in baseFolder (e.g. parent/child/folderName/img.jpg)
  var fileName = Utilities.formatDate(new Date(), "GMT", "yyyy-MM-dd-hh-mm-ss") + "-" + e.parameter.fileName;
  var file = e.parameter.file;

  // parse file
  var contentType = file.substring(file.indexOf(":")+1, file.indexOf(";")); // should be 'image/jpeg'
  var data = file.substring(file.indexOf(",")+1);

  // report to Google Docs log for debugging
  var message = `FILE: ${file.substring(0,40)}\n\
                TYPE: ${contentType}\n\
                DATA: ${file.substring(0,20)}`;
  log(message); // you can disable this
  
  // find folder or create if does not exist
  var folder = getDriveFolder(baseFolder + folderName);

  // convert data to image file and save to folder
  data = Utilities.base64Decode(data);
  var blob = Utilities.newBlob(data, contentType, fileName);
  var file = folder.createFile(blob);    
  file.setDescription("Uploaded by " + fileName);
  
  // return info
  var imageID = file.getUrl().substring(file.getUrl().indexOf("/d/")+3,file.getUrl().indexOf("view")-1);
  var imageUrl = "https://drive.google.com/uc?authuser=0&id="+imageID;
  return  ContentService.createTextOutput(folderName+"/"+fileName+"\n"+imageUrl);
}


// function created by Amit Agarwal: https://www.labnol.org/code/19925-google-drive-folder-path
function getDriveFolder(path) {

  var name, folder, search, fullPath;

  // remove extra slashes and trim the path
  fullPath = path.replace(/^\/*|\/*$/g, '').replace(/^\s*|\s*$/g, '').split("/");

  // always start with the main Drive folder
  folder = DriveApp.getRootFolder();

  for (var subFolder in fullPath) {

    name = fullPath[subFolder];
    search = folder.getFoldersByName(name);

    // if folder does not exit, create it in the current level
    folder = search.hasNext() ? search.next() : folder.createFolder(name);
  }

  return folder;
}

function log(message) {

  var fileName = 'log'; // the name of our log file (Google Docs)

  // grab files from base folder
  var folder = getDriveFolder(baseFolder);
  var files = folder.getFiles();
  

  var docFound = false;
  var doc;

  // open log document if exists in baseFolder; otherwise, create a new one
  while (files.hasNext()) {
    var file = files.next();
    var fileType = file.getMimeType();
    // check that fileName = 'log' and fileType = google-apps.document
    if(file.getName() == fileName && fileType == 'application/vnd.google-apps.document'){
      doc = DocumentApp.openById(file.getId());
      docFound = true;
    }
  }
  if (!docFound) doc = DocumentApp.create(fileName);

  // write message to log (newest first)
  var body = doc.getBody();
  var dateTime = Utilities.formatDate(new Date(), "GMT", "yyyy-MM-dd-hh-mm-ss");
  body.insertParagraph(0, dateTime + ": " + message); // 0 inserts at top line in document
}