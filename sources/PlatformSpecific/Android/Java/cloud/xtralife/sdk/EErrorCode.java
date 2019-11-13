package cloud.xtralife.sdk;

public enum EErrorCode {
	// Not following the naming convention, in order to match with the C++ code
	enNoErr(0),
	enExternalCommunityError(1012),
	enCanceled(1016),
	enLogicError(1024),
	enErrorWithExternalStore(1025),
	enUserCanceledPurchase(1026),
	enInternalError(1027);

	static EErrorCode fromCode(int code) {
		for (EErrorCode c: EErrorCode.values()) {
			if (c.code == code) { return c; }
		}
		// Unknown code
		return EErrorCode.enInternalError;
	}

	EErrorCode(int code) {
		this.code = code;
	}
	private int code;
	public int getCode() { return code; }
}
