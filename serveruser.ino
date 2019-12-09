

#ifndef USEROBJ
#define USEROBJ

#include <vector>

class User{
  public:
    String name;
    String token;
    char role;

    // dummy constructor for creating arrays:
    User(){
      name = "";
      token = "";
      role = '-';
    }

    // initialize user object
    User(String newName, char newRole, String newToken){
      name = newName;
      role = newRole;
      token = newToken;
    }

};

std::vector<User*> loggedInUsersVector;

// if name/password match, updates loggedInUsers and numLoggedInUsers
//    and returns index in loggedInUsers
// else returns -1
// if no dataset exists, temporarily adds entry with admin rights
// NOTE: use decodePercentString(inputString) for userName! (not for password)
int tryLogin(String userName, String pwd){
  // read module config from file
  if(SPIFFS.exists(USERSFILENAME)){
    // open config file in read mode
    File usersFile = SPIFFS.open(USERSFILENAME, "r");
    usersFile.setTimeout(50);
    // read each line and see, which configuration it is
    while(usersFile.available()){
      // every three lines are one user. Format: [username]\n[password]\n['role']
      // TODO: hash paswords!
      String name = usersFile.readStringUntil('\n');

      if((userName == name) && usersFile.available()){
        String password = usersFile.readStringUntil('\n');
        // TODO: hash password (Use ChipID and Flashchipid as salt? role as well?) with sha256(pwd + ...)
        // if password available AND further line available containing role
        if((password == pwd) && (usersFile.available())){

          for(byte i=0; i<loggedInUsersVector.size(); i++){
            // if the user is allready logged in
            if(loggedInUsersVector.at(i)->name == userName){
              // nothing to do except for setting a new token, closing the users file
              // and returning true
              loggedInUsersVector.at(i)->token = generateToken();
              usersFile.close();
              return i;
            }
          }
          // we need a new entry for the current user, so read its role as well
          String roleStr = usersFile.readStringUntil('\n');
          // close file
          usersFile.close();
          // in case something went wrong reading from the file/when the file was written,
          // make this user a basic user
          if(roleStr == ""){
            roleStr = "" + String(USERVIEWER);
          }

          // insert new User object into vector of users
          loggedInUsersVector.push_back(new User(name, roleStr.charAt(0), generateToken()));
          // done - close file and return true
          usersFile.close();
          // return ID of last element in loggedInUsersVector (the new one)
          return loggedInUsersVector.size()-1;

        }else{
          // password incorrect, no need to search further
          usersFile.close();
          return -1;
        }
      }
    }
    // no matching entry found for username
    usersFile.close();
    return -1;
  }else{
    // if no users file exists, temporarily add user with admin rights (in RAM only)
    // so everyone can log in to a blank module
    // NOTE: gets overwritten by every login attempt and on first login after creation of new user!
    loggedInUsersVector.push_back(new User(userName, USERADMIN, generateToken()));
    // return id - which is the ID of the last element in the vector of logged in users (size-1)
    return loggedInUsersVector.size() - 1;

  }
}

// returns user ID of a logged in user for a given token
// returns -1 if the token is wrong
int checkGetLoggedinUserIndex(String username, String token){
  for (byte i = 0; i < loggedInUsersVector.size(); i++) {
    if(loggedInUsersVector.at(i)->name == username){
      if(loggedInUsersVector.at(i)->token == token){
        // name and token match, so return the id in the array of logged in users
        return i;
      }
    }
  }
  // no match -> not logged in!
  return -1;
}

// logout function
boolean logout(String username, String token){
  for (std::vector<User*>::iterator iter = loggedInUsersVector.begin(); iter != loggedInUsersVector.end(); ++iter) {
    if((*iter)->name == username){
      if((*iter)->token == token){
        // name and token match, so remove the user from logged in users vector
        loggedInUsersVector.erase(iter);
        return true;
      }
    }
  }
  // no match -> not logged in!
  return false;
}


// adds user if no such entry exists
// if no user file exists, only adds an admin user
boolean addUser(String name, String password, char role){
  // if the name is an empty String and/or the role type does not apply, refuse user creation
  if((name == "") || ((role!=USERADMIN) && (role!=USERBASIC) && (role!=USERVIEWER))){
    return false;
  }
  if(SPIFFS.exists(USERSFILENAME)){
    // open config file in read mode
    File usersFile = SPIFFS.open(USERSFILENAME, "r");
    usersFile.setTimeout(50);
    // read names
    while(usersFile.available()){
      //read (next) name
      if(name == usersFile.readStringUntil('\n')){
        // name allready exists, close file, return false
        usersFile.close();
        return false;
      }
      // "consume" two more lines (password and role)
      if(usersFile.available()){
        usersFile.readStringUntil('\n');
        if(usersFile.available()){
          usersFile.readStringUntil('\n');
        }
      }
    }
    usersFile.close();
  }else{
    // make sure the first user is an admin user. If not, return false
    // (user file not yet existing => first user)
    if(role != 'a'){
      return false;
    }
  }
  // add user entry to file
  // important: append mode "a", we do not want to overwrite this file!
  File usersFile = SPIFFS.open(USERSFILENAME, "a");
  usersFile.print(name);
  usersFile.print("\n");
  usersFile.print(password);
  usersFile.print("\n");
  usersFile.print(role);
  usersFile.print("\n");
  usersFile.close();
  return true;
}

// updates the users role TODO: test!
boolean updateUserRole(String userName, char newRole){
  if(SPIFFS.exists(USERSFILENAME)){
    // open config file in read mode
    File usersFile = SPIFFS.open(USERSFILENAME, "r");
    usersFile.setTimeout(50);

    // else create new String to overwrite file with
    String newUsers = "";
    boolean nameFound = false;
    boolean otherAdminExists = false;

    while(usersFile.available()){
      String name = usersFile.readStringUntil('\n');
      // add name anyway
      newUsers += name + "\n";
      // if this name is ignore this line and the following two
      if(!name.equals(userName)){
        // add this users password and role to the new file
        if (usersFile.available()){
        // add password
          newUsers += usersFile.readStringUntil('\n') + "\n";
          if (usersFile.available()){
            // add role
            String role = usersFile.readStringUntil('\n');
            if(role.equals(USERADMINSTRING)){
              otherAdminExists = true;
            }
            newUsers += role + "\n";
          }
        }
      }else{
        nameFound = true;
        // we found our user
        if (usersFile.available()){
          // save password
          newUsers += usersFile.readStringUntil('\n') + "\n";
          if (usersFile.available()){
            // save new role
            newUsers += newRole + "\n";
          }
        }
      }
    }

    usersFile.close();

    // if we are actually updating a user entry (aka we found the user)
    // AND this user is either becoming admin OR there is another admin existing (don't degrade the last admin!)
    // save the constructed String
    // TODO: use multiple files parallel in case of power failure (load from one if only one of those exists)
    //        and when saving first save to the backup-file, then to the primary
    if(nameFound && (otherAdminExists || (newRole == USERADMIN))){
      // save new users string to new file
      SPIFFS.remove(USERSFILENAME);
      // let's hope the power does not fail until the first admin info is written
      usersFile = SPIFFS.open(USERSFILENAME, "w");
      usersFile.print(newUsers);
      usersFile.close();
      return true;
    }
    // user last admin or no user having the given username found
    return false;
  }
  // no file - no user to delete.
  return false;
}


// deletes the entry for the given user if
// a) entry exists
// b) entry is not the last admin user existing
boolean deleteUser(String nameToDelete){
  if(SPIFFS.exists(USERSFILENAME)){
    // open config file in read mode
    File usersFile = SPIFFS.open(USERSFILENAME, "r");
    usersFile.setTimeout(50);

    // else create new String to overwrite file with
    String newUsers = "";
    boolean otherAdminExists = false;
    boolean nameFound = false;

    while(usersFile.available()){
      String name = usersFile.readStringUntil('\n');
      // if this name is ignore this line and the following two
      if(!name.equals(nameToDelete)){
        // add this user, password and role to the new file
        newUsers += name + "\n";
          // add password
          newUsers += usersFile.readStringUntil('\n') + "\n";
          if (usersFile.available()){
            String role = usersFile.readStringUntil('\n');
            if(role.equals(USERADMINSTRING)){
              otherAdminExists = true;
            }
            // add role
            newUsers += role + "\n";
          }
      }else{
        nameFound = true;
        // ignore this user and discard the next two lines (password and role)
        if (usersFile.available()){
          // discard password
          usersFile.readStringUntil('\n');
          if (usersFile.available()){
            // discard role
            usersFile.readStringUntil('\n');
          }
        }
      }
    }

    usersFile.close();

    // if we are not up to deleting the last admin, and are actually up to delete a user,
    // save the constructed String (if the user is no admin, some admin user will exist anyway)
    // TODO: use multiple files parallel in case of power outage
    if(otherAdminExists && nameFound){
      // save new users string to new file
      SPIFFS.remove(USERSFILENAME);
      // let's hope the power does not fail until the first admin info is written
      usersFile = SPIFFS.open(USERSFILENAME, "w");
      usersFile.print(newUsers);
      usersFile.close();
      // TODO: check loggedInUsersVector for the deleted user and remove it from the vector
      return true;
    }
    // user last admin or no user having the given username found
    return false;
  }
  // no file - no user to delete.
  return false;
}

// updates a users password
boolean changePassword(String userName, String currPwd, String newPwd){
  if(SPIFFS.exists(USERSFILENAME)){
    // open config file in read mode
    File usersFile = SPIFFS.open(USERSFILENAME, "r");
    usersFile.setTimeout(50);

    // else create new String to overwrite file with
    String newUsers = "";
    boolean nameFound = false;

    while(usersFile.available()){
      String name = usersFile.readStringUntil('\n');
      // add name anyway
      newUsers += name + "\n";
      // if this name is ignore this line and the following two
      if(!name.equals(userName)){
        // add this users password and role to the new file
        if (usersFile.available()){
        // add password
          newUsers += usersFile.readStringUntil('\n') + "\n";
          if (usersFile.available()){
            // add role
            newUsers += usersFile.readStringUntil('\n') + "\n";
          }
        }
      }else{
        nameFound = true;
        // we found our user
        if (usersFile.available()){
          // check password, if it matches, replace by new password
          String pwd = usersFile.readStringUntil('\n');
          if(pwd.equals(currPwd)){
            newUsers += newPwd + "\n";
          }else{
            // password does not match
            return false;
          }
          if (usersFile.available()){
            // save role
            newUsers += usersFile.readStringUntil('\n') + "\n";
          }
        }
      }
    }

    usersFile.close();

    // if we are actually updating a user entry (aka we found the user)
    // save the constructed String
    // TODO: use multiple files parallel in case of power failure (load from one if only one of those exists)
    //        and when saving first save to the backup-file, then to the primary
    if(nameFound){
      // save new users string to new file
      SPIFFS.remove(USERSFILENAME);
      // let's hope the power does not fail until the first admin info is written
      usersFile = SPIFFS.open(USERSFILENAME, "w");
      usersFile.print(newUsers);
      usersFile.close();
      return true;
    }
    // user last admin or no user having the given username found
    return false;
  }
  // no file - no user to delete.
  return false;
}

/**
 * returns '-' when the user is not logged in
 * else returns the charrepresenting the role of a user
 */
char getUserRole(signed char loggedInId){
  // if no user, return '-'
  if(loggedInId < 0){
    return '-';
  }

  if(loggedInUsersVector.size() != 0){
    return loggedInUsersVector.at(loggedInId)->role;
  }
  return '-';
}

// generates asession token in a pseudo random way
String generateToken(){
  String token = String(millis());
  token += String(ESP.getChipId() + ESP.getFreeHeap() + ESP.getFlashChipId());
  token += String(millis() + ESP.getFlashChipId());
  return token;
}



String getUsersAsJsonArray(){
  // start JSON array
  String users = "[";
  File usersFile = SPIFFS.open(USERSFILENAME, "r");
  usersFile.setTimeout(50);
  boolean firstUser = true;

  while(usersFile.available()){
    // read name
    String name = usersFile.readStringUntil('\n');
    String role = "-";
    if(usersFile.available()){
      // read and ignore password
      usersFile.readStringUntil('\n');
      if (usersFile.available()){
        role = usersFile.readStringUntil('\n');
        role = role.substring(0,1);
      }
    }
    if(!firstUser){
      users += ",";
    }
    firstUser = false;
    users += "{\"name\":\"";
    users += formatForJson(name);
    users += "\",\"role\":\"";
    // read role
    users += role;
    users += "\"}";
  }

  usersFile.close();
  // close JSON array and return
  users += "]";
  return users;
}

String getRolesAsJsonArray(){
  return "[{name: \"Admin\", id: \"a\"},{name: \"Control\", role: \"c\"},{name: \"View\", role: \"v\"}]";
}



#endif
