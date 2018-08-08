/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


void MainController::GlobalAsyncModuleHandler::removeAsync(Processor* p, const SafeFunctionCall::Function& removeFunction)
{
	if (removeFunction)
	{
		auto f = [removeFunction](Processor* p)
		{
			LockHelpers::freeToGo(p->getMainController());

			auto result = removeFunction(p);

			p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, What::Delete);

			return result;
		};

		mc->getKillStateHandler().killVoicesAndCall(p, f, KillStateHandler::SampleLoadingThread);
	}
	else
	{
		p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, What::Delete);
	}
}

void MainController::GlobalAsyncModuleHandler::addAsync(Processor* p, const SafeFunctionCall::Function& addFunction)
{
	auto f = [addFunction](Processor* p)
	{
		auto result = addFunction(p);

		p->getMainController()->getGlobalAsyncModuleHandler().addPendingUIJob(p, What::Add);

		return result;
	};

	p->getMainController()->getKillStateHandler().killVoicesAndCall(p, f, KillStateHandler::SampleLoadingThread);
}


void MainController::GlobalAsyncModuleHandler::addPendingUIJob(Processor* p, What what)
{
	if (what == Add)
	{
		auto f = [](Dispatchable* obj)
		{
			auto p = static_cast<Processor*>(obj);
			auto parent = p->getParentProcessor(false);

			if (parent != nullptr)
				parent->sendRebuildMessage(true);

			return Dispatchable::Status::OK;
		};

		mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p, f);
	}
	else
	{
		auto f = [](Dispatchable* obj)
		{
			auto p = static_cast<Processor*>(obj);
			p->sendDeleteMessage();

			auto parent = p->getParentProcessor(false, false);

			if (parent != nullptr)
				parent->sendRebuildMessage(true);

			delete p;

			return Dispatchable::Status::OK;
		};

		p->setIsWaitingForDeletion();

		mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(p, f);
	}
}

} // namespace hise
