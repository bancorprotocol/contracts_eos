module.exports = Object.freeze({
    ERRORS: {
        PERMISSIONS: 'Missing required authority',
        POSITIVE_TRANSFER: 'must transfer positive quantity',
        CONVERT_SELF: 'cannot convert to self',
        CONVERTER_DISABLED: 'converter is disabled',
        FEE_TOO_HIGH: 'fee must be lower or equal to 1000',
        MAX_FEE_TOO_HIGH: 'maximum fee must be lower or equal to 1000',
        FEE_OVER_MAX: 'fee must be lower or equal to the maximum fee',
        PRECISION_MISMATCH: 'symbol precision mismatch',
        OVER_SPENDING: 'overdrawn balance',
        INVALID_RATIO: 'ratio must be between 1 and 1000',
        NO_KEY: 'unable to find key',
        DUPLICATE_TRANSACTION: 'Duplicate transaction',
        REPORTER_ALREADY_DEFINED: 'reporter already defined',
        REPORTER_DOESNT_EXIST: 'reporter does not exist',
        TRANSFER_DATA_MISMATCH: 'transfer data doesn\'t match',
        SINGLETON_DOESNT_EXIST: 'singleton does not exist',
        REROUTING_DISABLED: 'transaction rerouting is disabled'
    }
});