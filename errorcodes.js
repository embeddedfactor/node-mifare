var errorCodes = {
  0x00000000: { "text": "No error has occurred.", "title": "SCARD_S_SUCCESS", "retry": false},
  0x80100001: { "text": "An internal consistency check failed.", "title": "SCARD_F_INTERNAL_ERROR", "retry": false},
  0x80100002: { "text": "The action was canceled by a Cancel request.", "title": "SCARD_E_CANCELLED", "retry": false},
  0x80100003: { "text": "The supplied handle was invalid.", "title": "SCARD_E_INVALID_HANDLE", "retry": false},
  0x80100004: { "text": "One or more of the supplied parameters could not be properly interpreted.", "title": "SCARD_E_INVALID_PARAMETER", "retry": false},
  0x80100005: { "text": "Registry startup information is missing or invalid.", "title": "SCARD_E_INVALID_TARGET", "retry": false},
  0x80100006: { "text": "Not enough memory available to complete this command.", "title": "SCARD_E_NO_MEMORY", "retry": false},
  0x80100007: { "text": "An internal consistency timer has expired.", "title": "SCARD_F_WAITED_TOO_LONG", "retry": true},
  0x80100008: { "text": "The data buffer to receive returned data is too small for the returned data.", "title": "SCARD_E_INSUFFICIENT_BUFFER", "retry": false},
  0x80100009: { "text": "The specified reader name is not recognized.", "title": "SCARD_E_UNKNOWN_READER", "retry": false},
  0x8010000A: { "text": "The user-specified time-out value has expired.", "title": "SCARD_E_TIMEOUT", "retry": true},
  0x8010000B: { "text": "The smart card cannot be accessed because of other connections outstanding.", "title": "SCARD_E_SHARING_VIOLATION", "retry": true},
  0x8010000C: { "text": "The operation requires a smart card, but no smart card is currently in the device.", "title": "SCARD_E_NO_SMARTCARD", "retry": false},
  0x8010000D: { "text": "The specified smart card name is not recognized.", "title": "SCARD_E_UNKNOWN_CARD", "retry": false},
  0x8010000E: { "text": "The system could not dispose of the media in the requested manner.", "title": "SCARD_E_CANT_DISPOSE", "retry": false},
  0x8010000F: { "text": "The requested protocols are incompatible with the protocol currently in use with the smart card.", "title": "SCARD_E_PROTO_MISMATCH", "retry": false},
  0x80100010: { "text": "The reader or smart card is not ready to accept commands.", "title": "SCARD_E_NOT_READY", "retry": true},
  0x80100011: { "text": "One or more of the supplied parameters values could not be properly interpreted.", "title": "SCARD_E_INVALID_VALUE", "retry": false},
  0x80100012: { "text": "The action was canceled by the system, presumably to log off or shut down.", "title": "SCARD_E_SYSTEM_CANCELLED", "retry": false},
  0x80100013: { "text": "An internal communications error has been detected.", "title": "SCARD_F_COMM_ERROR", "retry": false},
  0x80100014: { "text": "An internal error has been detected, but the source is unknown.", "title": "SCARD_F_UNKNOWN_ERROR", "retry": false},
  0x80100015: { "text": "An ATR obtained from the registry is not a valid ATR string.", "title": "SCARD_E_INVALID_ATR", "retry": false},
  0x80100016: { "text": "An attempt was made to end a non-existent transaction.", "title": "SCARD_E_NOT_TRANSACTED", "retry": false},
  0x80100017: { "text": "The specified reader is not currently available for use.", "title": "SCARD_E_READER_UNAVAILABLE", "retry": false},
  0x80100018: { "text": "The operation has been stopped to allow the server application to exit.", "title": "SCARD_P_SHUTDOWN", "retry": false},
  0x80100019: { "text": "The PCI Receive buffer was too small.", "title": "SCARD_E_PCI_TOO_SMALL", "retry": false},
  0x80100020: { "text": "No primary provider can be found for the smart card.", "title": "SCARD_E_ICC_INSTALLATION", "retry": false},
  0x80100021: { "text": "The requested order of object creation is not supported.", "title": "SCARD_E_ICC_CREATEORDER", "retry": false},
  0x80100022: { "text": "This smart card does not support the requested feature.", "title": "SCARD_E_UNSUPPORTED_FEATURE", "retry": false},
  0x80100023: { "text": "The specified directory does not exist in the smart card.", "title": "SCARD_E_DIR_NOT_FOUND", "retry": false},
  0x80100024: { "text": "The specified file does not exist in the smart card.", "title": "SCARD_E_FILE_NOT_FOUND", "retry": false},
  0x80100025: { "text": "The supplied path does not represent a smart card directory.", "title": "SCARD_E_NO_DIR", "retry": false},
  0x8010001A: { "text": "The reader device driver does not meet minimal requirements for support.", "title": "SCARD_E_READER_UNSUPPORTED", "retry": false},
  0x8010001B: { "text": "The reader device driver did not produce a unique reader name.", "title": "SCARD_E_DUPLICATE_READER", "retry": false},
  0x8010001C: { "text": "The smart card does not meet minimal requirements for support.", "title": "SCARD_E_CARD_UNSUPPORTED", "retry": false},
  0x8010001D: { "text": "Smart Cards for Windows is not running.", "title": "SCARD_E_NO_SERVICE", "retry": false},
  0x8010001E: { "text": "Smart Cards for Windows has shut down.", "title": "SCARD_E_SERVICE_STOPPED", "retry": false},
  0x8010001F: { "text": "An unexpected card error has occurred.", "title": "SCARD_E_UNEXPECTED", "retry": false},
  0x80100026: { "text": "The supplied path does not represent a smart card file.", "title": "SCARD_E_NO_FILE", "retry": false},
  0x80100027: { "text": "Access is denied to this file.", "title": "SCARD_E_NO_ACCESS", "retry": false},
  0x80100028: { "text": "The smart card does not have enough memory to store the information.", "title": "SCARD_E_WRITE_TOO_MANY", "retry": false},
  0x80100029: { "text": "There was an error trying to set the smart card file object pointer.", "title": "SCARD_E_BAD_SEEK", "retry": false},
  0x8010002A: { "text": "The supplied PIN is incorrect.", "title": "SCARD_E_INVALID_CHV", "retry": false},
  0x8010002B: { "text": "An unrecognized error code was returned from a layered component.", "title": "SCARD_E_UNKNOWN_RES_MSG", "retry": false},
  0x8010002C: { "text": "The requested certificate does not exist.", "title": "SCARD_E_NO_SUCH_CERTIFICATE", "retry": false},
  0x8010002D: { "text": "The requested certificate could not be obtained.", "title": "SCARD_E_CERTIFICATE_UNAVAILABLE", "retry": false},
  0x8010002E: { "text": "Cannot find a smart card reader.", "title": "SCARD_E_NO_READERS_AVAILABLE", "retry": false},
  0x8010002F: { "text": "A communications error with the smart card has been detected. Retry the operation.", "title": "SCARD_E_COMM_DATA_LOST", "retry": false},
  0x80100030: { "text": "The requested key container does not exist.", "title": "SCARD_E_NO_KEY_CONTAINER", "retry": false},
  0x80100031: { "text": "Smart Cards for Windows is too busy to complete this operation.", "title": "SCARD_E_SERVER_TOO_BUSY", "retry": false},
  0x80100032: { "text": "The smart card PIN cache has expired.", "title": "SCARD_E_PIN_CACHE_EXPIRED", "retry": false},
  0x80100033: { "text": "The smart card PIN cannot be cached.", "title": "SCARD_E_NO_PIN_CACHE", "retry": false},
  0x80100034: { "text": "The smart card is read-only and cannot be written to.", "title": "SCARD_E_READ_ONLY_CARD", "retry": false},
  0x80100065: { "text": "The reader cannot communicate with the smart card due to ATR configuration conflicts.", "title": "SCARD_W_UNSUPPORTED_CARD", "retry": false},
  0x80100066: { "text": "The smart card is not responding to a reset.", "title": "SCARD_W_UNRESPONSIVE_CARD", "retry": false},
  0x80100067: { "text": "Power has been removed from the smart card, so that further communication is impossible.", "title": "SCARD_W_UNPOWERED_CARD", "retry": false},
  0x80100068: { "text": "The smart card has been reset, so any shared state information is invalid.", "title": "SCARD_W_RESET_CARD", "retry": false},
  0x80100069: { "text": "The smart card has been removed, so that further communication is impossible.", "title": "SCARD_W_REMOVED_CARD", "retry": false},
  0x8010006A: { "text": "Access was denied because of a security violation.", "title": "SCARD_W_SECURITY_VIOLATION", "retry": false},
  0x8010006B: { "text": "The card cannot be accessed because the wrong PIN was presented.", "title": "SCARD_W_WRONG_CHV", "retry": false},
  0x8010006C: { "text": "The card cannot be accessed because the maximum number of PIN entry attempts has been reached.", "title": "SCARD_W_CHV_BLOCKED", "retry": false},
  0x8010006D: { "text": "The end of the smart card file has been reached.", "title": "SCARD_W_EOF", "retry": false},
  0x8010006E: { "text": "The action was canceled by the user.", "title": "SCARD_W_CANCELLED_BY_USER", "retry": false},
  0x8010006F: { "text": "No PIN was presented to the smart card.", "title": "SCARD_W_CARD_NOT_AUTHENTICATED", "retry": false},
  0x80100070: { "text": "The requested item could not be found in the cache.", "title": "SCARD_W_CACHE_ITEM_NOT_FOUND", "retry": false},
  0x80100071: { "text": "The requested cache item is too old and was deleted from the cache.", "title": "SCARD_W_CACHE_ITEM_STALE", "retry": false},
  0x80100072: { "text": "The new cache item exceeds the maximum per-item size defined for the cache.", "title": "SCARD_W_CACHE_ITEM_TOO_BIG", "retry": false},
};

function retry(callback) {
  for(var i=3;i!=0;i--) {
    var ret = callback();
    if(ret['code']==0) {
      return ret;
    }
    var err = errorCodes[ret['res'].toString()];
    if(!err['retry']) {
      return ret;
    }
  }
  return ret;
}

