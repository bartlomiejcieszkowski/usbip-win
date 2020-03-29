#include "csq_with_thread.h"

VOID csq_with_thread_complete_cancelled_irp(_In_ PIO_CSQ csq, _In_ PIRP irp)
{
	irp->IoStatus.Status = STATUS_CANCELLED;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
}

_Acquires_lock_(CONTAINING_RECORD(csq, csq_with_thread, irp_csq)->irp_csq_lock)
VOID csq_with_thread_acquire_lock(_In_ PIO_CSQ csq, _Out_ PKIRQL irql)
{
	pcsq_with_thread ctx = CONTAINING_RECORD(csq, csq_with_thread, irp_csq);
	KeAcquireSpinLock(&ctx->irp_csq_lock, irql);
}

_Releases_lock_(CONTAINING_RECORD(csq, csq_with_thread, irp_csq)->irp_csq_lock)
VOID csq_with_thread_release_lock(_In_ PIO_CSQ csq, _In_ KIRQL irql)
{
	pcsq_with_thread ctx = CONTAINING_RECORD(csq, csq_with_thread, irp_csq);
	KeReleaseSpinLock(&ctx->irp_csq_lock, irql);
}

VOID csq_with_thread_insert_irp(_In_ PIO_CSQ csq, _In_ PIRP irp)
{
	pcsq_with_thread ctx = CONTAINING_RECORD(csq, csq_with_thread, irp_csq);
	InsertTailList(&ctx->irp_csq_list, &irp->Tail.Overlay.ListEntry);
}

VOID csq_with_thread_remove_irp(_In_ PIO_CSQ csq, _In_ PIRP irp)
{
	UNREFERENCED_PARAMETER(csq);
	RemoveEntryList(&irp->Tail.Overlay.ListEntry);
}

PIRP csq_with_thread_peek_next_irp(PIO_CSQ csq, PIRP irp, PVOID peek_context)
{
	UNREFERENCED_PARAMETER(peek_context);
	pcsq_with_thread ctx = CONTAINING_RECORD(csq, csq_with_thread, irp_csq);
	PLIST_ENTRY next_entry = NULL;
	PIRP next_irp = NULL;
	PIO_STACK_LOCATION irp_stack = NULL;

	if (irp == NULL) {
		next_entry = &ctx->irp_csq_list.Flink;
	}
	else {
		next_entry = irp->Tail.Overlay.ListEntry.Flink;
	}

	// we explicitly set context to NULL, so there lookup of first matching
	return (next_entry != &ctx->irp_csq_list)
		? CONTAINING_RECORD(next_entry, IRP, Tail.Overlay.ListEntry)
		: NULL;
}

VOID csq_with_thread_main(_In_ PVOID context)
{
	pcsq_with_thread ctx = (pcsq_with_thread)context;
	PIRP irp;
	NTSTATUS status;

	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

	while (TRUE)
	{
		KeWaitForSingleObject(&ctx->thread_semaphore, Executive, KernelMode, FALSE, NULL);
		if (ctx->thread_stop) {
			PsTerminateSystemThread(STATUS_SUCCESS);
		}

		irp = IoCsqRemoveNextIrp(&ctx->irp_csq, NULL);
		if (irp == NULL) {
			continue;
		}

		// processing must be done inside this method and result in IoCompleteRequest
		ctx->process_irp_fn(ctx->process_irp_ctx, irp);
	}
}
